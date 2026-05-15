#include <stdio.h>
#include <stdlib.h>
#include "hash.h"

unsigned int hash_function(CompositeKey key){
    unsigned long hash = key.CodigoEstacionNuevo;
    hash = hash * 41 + key.CodigoSensor;
    hash = hash * 41 + key.FechaObservacionNum;
    return hash % TABLE_SIZE;
}

int hash_insert(FILE *f, CompositeKey key, long offset){
    HashNode node;
    Bucket *b = malloc(sizeof(Bucket));

    unsigned int index = hash_function(key);
    fseek(f, index * sizeof(Bucket), SEEK_SET);
    fread(b, sizeof(Bucket), 1, f);
    
    node.key = key;
    node.offset = offset;
    node.next = b->first;

    fseek(f, 0, SEEK_END);
    long node_offset = ftell(f);

    fwrite(&node, sizeof(HashNode), 1, f);

    b->first = node_offset;

    long bucket_offset = index * sizeof(Bucket);
    fseek(f, bucket_offset, SEEK_SET);
    fwrite(b, sizeof(Bucket), 1, f);
    free(b);
    return 0;
}

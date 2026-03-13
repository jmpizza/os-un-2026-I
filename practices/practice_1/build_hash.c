#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "hash.h"


#define BLOCK 200000

Bucket hashBuckets[TABLE_SIZE];

int insert(FILE *f, CompositeKey key, long offset){
    HashNode node;
    unsigned int index = hash_function(key);

    Bucket *b = &hashBuckets[index];

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

    return 0;
}
void create_index(const char *filename){
    for(int i = 0; i < TABLE_SIZE; i++){
        hashBuckets[i].first = -1;
    }

    FILE *f = fopen(filename,"wb");
    if(!f){
        perror("Error creando índice");
        exit(-1);
    }
    for(int i = 0; i < TABLE_SIZE; i++){
        fwrite(&hashBuckets[i], sizeof(Bucket), 1, f);
    }
    fclose(f);
}

CompositeKey extract_key(Record r){
    CompositeKey key;
    key.CodigoEstacionNuevo = r.CodigoEstacionNuevo;
    key.CodigoSensor = r.CodigoSensor;
    key.FechaObservacionNum = r.FechaObservacionNum;
    return key;
}

int build_index() {
    FILE *csv = fopen("data.csv", "r");
    FILE *index = fopen("data_index.dat", "r+b");

    long offset;
    char header[300];
    Record r;
    CompositeKey key;
    
    if(!csv) {
        perror("Error al abrir el archivo CSV: ");
        exit(-1);
    }

    fgets(header, sizeof(header), csv);

    while(true){
        offset = ftell(csv);
        if(offset == -1){
            perror("Error obteniendo la posicion del CSV: ");
            exit(-1);
        }
        if (fscanf(csv, "%ld,%d,%29[^,],%f,%59[^,],%59[^,],%29[^,],%34[^,],%f,%f,%34[^,],%4[^,],%d,%ld",
            &r.CodigoEstacion, &r.CodigoSensor, r.FechaObservacion,&r.ValorObservado, r.NombreEstacion,
            r.Departamento, r.Municipio, r.ZonaHidrografica, &r.Latitud, &r.Longitud,
            r.DescripcionSensor, r.UnidadMedida, &r.CodigoEstacionNuevo, &r.FechaObservacionNum) != 14){
                break;
        }
        
        key = extract_key(r);

        insert(index, key, offset);
    }
    
    fclose(index);
    fclose(csv);

    //save_index();
    return 0;
}

int main(){
    create_index("data_index.dat");

    build_index();

    return 0;
}
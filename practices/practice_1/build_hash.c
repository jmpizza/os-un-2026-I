#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include "hash.h"

#define BLOCK 200000

HashNode hashTable[TABLE_SIZE];

int insert(CompositeKey key, long offset){
    unsigned int index = hash_function(key);
    HashNode *node = &hashTable[index];

    while (node->next != NULL){
        node = node->next;
    }

    node->key = key;
    node->offset = offset;
    node->next = NULL;
    return 0;
}

CompositeKey extract_key(Record r){
    CompositeKey key;
    key.CodigoEstacionNuevo = r.CodigoEstacionNuevo;
    key.CodigoSensor = r.CodigoSensor;
    key.FechaObservacionNum = r.FechaObservacionNum;
    return key;
}

int save_index(){
    FILE *index = fopen("data_index.dat", "w");
    
    if(!index){
        perror("Error al abrir el archivo indice: ");
        exit(-1);
    }

    fwrite(hashTable, sizeof(HashNode), TABLE_SIZE, index);

    fclose(index);

    return 0;
}

int build_index() {
    FILE *csv = fopen("data.csv", "r");
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
        
        insert(key, offset);
    }
    
    fclose(csv);

    save_index();
    return 0;
}

int main(){
    build_index();

    return 0;
}
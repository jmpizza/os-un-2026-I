#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

#define TABLE_SIZE 2000003


typedef struct {
    long CodigoEstacion;
    int CodigoSensor;
    char FechaObservacion[30];
    float ValorObservado;
    char NombreEstacion[60];
    char Departamento[60];
    char Municipio[30];
    char ZonaHidrografica[35];
    float Latitud;
    float Longitud;
    char DescripcionSensor[35];
    char UnidadMedida[5];
    int CodigoEstacionNuevo;
    long FechaObservacionNum;
} Record;

typedef struct {
    int CodigoEstacion;
    int CodigoSensor;
    long FechaObservacionNum;
} CompositeKey;

typedef struct {
    CompositeKey key;
    long offset;
    int occupied;
} HashEntry;

HashEntry hashTable[TABLE_SIZE];

unsigned int hash_function(CompositeKey key){
    unsigned long hash = key.CodigoEstacion;
    hash = hash * 41 + key.CodigoSensor;
    hash = hash * 41 + key.FechaObservacionNum;
    return hash % TABLE_SIZE;
}

int insert(CompositeKey key, long offset){
    unsigned int index = hash_function(key);

    while (hashTable[index].occupied){
        index = (index + 1) % TABLE_SIZE;
    }

    hashTable[index].key = key;
    hashTable[index].offset = offset;
    hashTable[index].occupied = 1;
    return 0;
}

CompositeKey extract_key(Record r){
    CompositeKey key;
    key.CodigoEstacion = r.CodigoEstacion;
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

    fwrite(hashTable, sizeof(HashEntry), sizeof(hashTable), index);

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
            r.DescripcionSensor, r.UnidadMedida, &r.CodigoEstacion, &r.FechaObservacionNum) != 15){
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
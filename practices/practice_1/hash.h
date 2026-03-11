#include <stdint.h>
#ifndef HASH_H
#define HASH_H

#define TABLE_SIZE 2000003

typedef struct Record{
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
    int64_t FechaObservacionNum;
} Record;

typedef struct {
    int CodigoEstacionNuevo;
    int CodigoSensor;
    int64_t FechaObservacionNum;
} CompositeKey;

typedef struct HashNode{
    CompositeKey key;
    int64_t offset;
    struct HashNode *next;
} HashNode;

unsigned int hash_function(CompositeKey key);

#endif
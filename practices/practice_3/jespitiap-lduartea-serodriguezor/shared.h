#include <stdint.h>
#ifndef SHARED_H
#define SHARED_H
#include "hash.h"

#define filenamedata "data.csv"
#define index_primarykey "data_index.dat"

typedef enum {
    OP_BUSQUEDA_LLAVE = 1,
    OP_BUSQUEDA_REGISTRO = 2,
    OP_CREAR_REGISTRO = 3,
    OP_SALIR = 4
} Operacion;

typedef struct{
    Operacion op; //Que operación solicita
    CompositeKey key; //key
    int64_t RecordNum;
    float ValorObservado;
    char NombreEstacion[60];
    char Departamento[60];
    char Municipio[30];
    char ZonaHidrografica[35];
    float Latitud;
    float Longitud;
} Request;


typedef struct {
	Request ResInfo;		//operacion , key , valor observado
    int status;             // 0 = OK, 1 = error
} Response;



#endif
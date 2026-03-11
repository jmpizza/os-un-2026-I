#include "hash.h"

unsigned int hash_function(CompositeKey key){
    unsigned long hash = key.CodigoEstacionNuevo;
    hash = hash * 41 + key.CodigoSensor;
    hash = hash * 41 + key.FechaObservacionNum;
    return hash % TABLE_SIZE;
}
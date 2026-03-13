#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "hash.h"
#include "shared.h"
#include <stdbool.h>

HashNode *hashNode = NULL;

Record search(CompositeKey *key, const char *csvFile, const char *indexFile) {
    Record r = {0};
    Bucket b;
    HashNode node;
    unsigned int idx = hash_function(*key);

    FILE *index = fopen(indexFile, "rb");
    if (!index) {
        perror("Error abriendo indice");
        return r;
    }

    FILE *csv = fopen(csvFile, "r");
    if (!csv) {
        perror("Error abriendo CSV");
        fclose(index);
        return r;
    }

    long bucket_offset = idx * sizeof(Bucket);
    fseek(index, bucket_offset, SEEK_SET);
    fread(&b, sizeof(Bucket), 1, index);

    if (b.first == -1) {
        fclose(index);
        fclose(csv);
        return r;
    }

    long node_offset = b.first;

    while (node_offset != -1) {

        fseek(index, node_offset, SEEK_SET);
        if (fread(&node, sizeof(HashNode), 1, index) != 1)
            break;

        if (node.key.CodigoEstacionNuevo == key->CodigoEstacionNuevo &&
            node.key.CodigoSensor == key->CodigoSensor &&
            node.key.FechaObservacionNum == key->FechaObservacionNum) {

            printf("Offset encontrado %lld\n", (long long)node.offset);

            if (fseek(csv, node.offset, SEEK_SET) != 0) {
                perror("Error obteniendo registro CSV");
                break;
            }

            char raw[256];
            size_t n = fread(raw, 1, sizeof(raw)-1, csv);
            raw[n] = '\0';

            sscanf(raw,
                "%ld,%d,%29[^,],%f,%59[^,],%59[^,],%29[^,],%34[^,],%f,%f,%34[^,],%4[^,],%d,%ld",
                &r.CodigoEstacion,&r.CodigoSensor,r.FechaObservacion,&r.ValorObservado,
                r.NombreEstacion,r.Departamento,r.Municipio,r.ZonaHidrografica,&r.Latitud,
                &r.Longitud,r.DescripcionSensor,r.UnidadMedida,&r.CodigoEstacionNuevo,&r.FechaObservacionNum
            );

            fclose(index);
            fclose(csv);
            return r;
        }

        node_offset = node.next;
    }

    fclose(index);
    fclose(csv);

    return r;
}

int main(){
    int shm_id;
    struct Datos *ptr;
    CompositeKey key;
    Record r;

    printf("Buscador inicializa...\n");

    shm_id = shmget(1234, sizeof(struct Datos), IPC_CREAT | 0666);
    if(shm_id < 0) {
        perror("Error creando memoria compartida: ");
        exit(-1);
    }

    ptr = (struct Datos *) shmat(shm_id, NULL, 0);
    if(ptr == (struct Datos *) -1) {
        perror("Error en shmat: ");
        exit(-1);
    }
    
    CompositeKey *keyPtr;
    keyPtr = (CompositeKey *)malloc(sizeof(CompositeKey));
    if (keyPtr == NULL){
        perror("Error en keyPtr");
        exit(-1);
    }

    while(true) {
        if (ptr->listo == 1){
            keyPtr->CodigoEstacionNuevo = ptr->CodigoEstacionNuevo_id;
            keyPtr->CodigoSensor = ptr->CodigoSensor_id;
            keyPtr->FechaObservacionNum = ptr->FechaObservacionNum_id;

            printf("Informacion a Buscar Est=%d Sen=%d Fecha=%ld\n",
                keyPtr->CodigoEstacionNuevo, keyPtr->CodigoSensor, keyPtr->FechaObservacionNum
            );

            r = search(keyPtr, "data.csv", "data_index.dat");
            ptr->resultado = r.ValorObservado;
            printf("Valor observado: %.2f\n", r.ValorObservado);
            ptr->listo = 2;
        }
        usleep(1000);
    }

    shmdt(ptr);
    free(keyPtr);
    
    return 0;
}
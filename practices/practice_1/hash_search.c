#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "hash.h"
#include "shared.h"

HashNode *hashTable = NULL;

void load_index(const char *filename) {
    FILE *f = fopen(filename, "rb");
    if(!f) {
        perror("Error al abrir indice binario: ");
        exit(-1);
    }

    hashTable = malloc(TABLE_SIZE * sizeof(HashNode));
    if(!hashTable) {
        perror("Error reservando memoria para la tabla hash: ");
        exit(-1);
    }

    size_t read_count = fread(hashTable, sizeof(HashNode), TABLE_SIZE, f);
    printf("Entradas leidas: %zu\n", read_count);

    fclose(f);
}

Record search(CompositeKey key, const char *csvFile) {
    int err;
    size_t n;
    char raw[256];
    Record r = {0};
    unsigned int idx = hash_function(key);
    HashNode node = hashTable[idx];

    while(true) {
        if (node.key.CodigoEstacionNuevo == key.CodigoEstacionNuevo &&
            node.key.CodigoSensor == key.CodigoSensor &&
            node.key.FechaObservacionNum == key.FechaObservacionNum) { 
                
            printf("Offset encontrado %lld\n", (long long)node.offset);

            FILE *csv = fopen(csvFile, "r");
            if (!csv){
                perror("Error al abrrir el CSV: ");
                exit(-1);
            }
            printf("Buscando -> Est=%d Sen%d Fecha=%ld Offset=%ld\n",
                    key.CodigoEstacionNuevo,
                    key.CodigoSensor,
                    (long long)key.FechaObservacionNum,
                    node.offset
            );

            err = fseek(csv, node.offset, SEEK_SET);
            if(err == -1){
                perror("Error obteniendo el registro: ");
                exit(-1);
            }
            n = fread(raw, 1, 120, csv);
            raw[n] = '\0';

            sscanf(raw, "%ld,%d,%29[^,],%f,%59[^,],%59[^,],%29[^,],%34[^,],%f,%f,%34[^,],%4[^,],%d,%ld",
                &r.CodigoEstacion, &r.CodigoSensor, r.FechaObservacion,&r.ValorObservado, r.NombreEstacion,
                r.Departamento, r.Municipio, r.ZonaHidrografica, &r.Latitud, &r.Longitud,
                r.DescripcionSensor, r.UnidadMedida, &r.CodigoEstacion, &r.FechaObservacionNum
            );

            fclose(csv);
            return r;
        }
        if (node.next == NULL) break;
        node = *node.next;
    }

    return r;
}

int main(){
    int shm_id;
    struct Datos *ptr;
    CompositeKey key;
    Record r;

    printf("Buscador inicializa...\n");

    load_index("data_index.dat");

    printf("Indice cargado! \n");

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

    while(true) {
        if (ptr->listo == 1){
            CompositeKey key;
            key.CodigoEstacionNuevo = ptr->CodigoEstacionNuevo_id;
            key.CodigoSensor = ptr->CodigoSensor_id;
            key.FechaObservacionNum = ptr->FechaObservacionNum_id;

            printf("Informacion a Buscar Est=%d Sen=%d Fecha=%ld\n",
                key.CodigoEstacionNuevo, key.CodigoSensor, key.FechaObservacionNum
            );

            r = search(key, "data.csv");
            ptr->resultado = r.ValorObservado;
            printf("Valor observado: %.2f\n", r.ValorObservado);
            ptr->listo = 2;
        }
        usleep(1000);
    }

    shmdt(ptr);
    
    return 0;
}
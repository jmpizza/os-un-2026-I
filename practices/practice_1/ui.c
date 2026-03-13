#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>   // para usleep
#include "shared.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <inttypes.h>

int v_CodigoEstacion = -1;
int v_CodigoSensor = -1;
int64_t v_FechaObservacionNum = -1;

void BorrarPantalla() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void ImprimeMensaje(const char *mensaje) {
    printf("%s\n", (char *)mensaje);
    #ifdef _WIN32
        system("pause");
    #else
        printf("Presione ENTER para continuar...");
        getchar();
    #endif
}

void Menu() {
    printf("----------------------------------\n");
    printf("          Bienvenido\n");
    printf("Temperatura Ambiente del Aire\n");
    printf("ESTACION: %d SENSOR: %d FECHA: %ld\n", v_CodigoEstacion, v_CodigoSensor, v_FechaObservacionNum);
    printf("----------------------------------\n");
    printf("1. Ingresar Codigo de Estacion\n");
    printf("2. Ingresar Codigo de Sensor\n");
    printf("3. Ingresar Codigo de Fecha\n");
    printf("4. Buscar Valor observado\n");
    printf("5. Salir del Programa\n");
}

int Muestra_Menu() {
    int val_user;
    int result;

    while (true) {
        BorrarPantalla();
        Menu();
        printf("\n\tDigite una opcion <1-5>: ");
        result = scanf("%d", &val_user);

        while (getchar() != '\n'); // limpiar buffer

        if (result != 1 || val_user < 1 || val_user > 5) {
            ImprimeMensaje("\t<Valor equivocado> Digite un valor entre 1 y 5");
        } else {
            break;
        }
    }
    return val_user;
}

bool valida_dato(const char *tipo, int64_t  valor) {
    if (tipo[0] == 'E')  {
        return (valor >= 100 && valor <= 513);
    } else if (tipo[0] == 'S') {
        return (valor == 68 || valor == 71);
    }else if (tipo[0] == 'F') {
        return (valor >= 202501010000LL && valor <= 202511302358LL);
    }
    return false;
}

int main() {
    int *opcion;
    opcion = (int *) malloc(sizeof(int));
    if (opcion == NULL){
        perror("error en opción");
        exit(-1);
    }

    char *mensaje;
    mensaje = (char *) malloc(250 * sizeof(char));
    if (mensaje == NULL){
        perror("Error en mensaje");
        exit(-1);
    }

    // Crear/obtener memoria compartida al inicio

    int shm_id = shmget(1234, sizeof(struct Datos), IPC_CREAT | 0666);

    if (shm_id < 0) {
        perror("Error creando memoria compartida");
        exit(1);
    }

    struct Datos *ptr = (struct Datos *) shmat(shm_id, NULL, 0);
    if (ptr == (struct Datos *) -1) {
        perror("Error en shmat");
        exit(1);
    }

    while (true) {
        *opcion = Muestra_Menu();

        if (*opcion == 5) { // salir
            printf("\n\tGracias por utilizar el programa\n");
            shmdt(ptr); // desasociar
            shmctl(shm_id, IPC_RMID, NULL); // eliminar segmento
            break;

        } else if (*opcion == 1) {
            while (true) {
                printf("\rIngrese ID de Estacion (100-513): ");
                if (scanf("%d", &v_CodigoEstacion) != 1 || !valida_dato("E", v_CodigoEstacion)) {
                    while (getchar() != '\n'); // limpiar buffer
                    ImprimeMensaje("Dato invalido");
                    continue;
                }
                break;
            }
        } else if (*opcion == 2) {
            while (true) {
                printf("\rIngrese ID de Sensor (68/71): ");
                if (scanf("%d", &v_CodigoSensor) != 1 || !valida_dato("S", v_CodigoSensor)) {
                    while (getchar() != '\n');
                    ImprimeMensaje("Dato invalido");
                    continue;
                }
                break;
            }
        } else if (*opcion == 3) {
            while (true) {
                printf("\rIngrese fecha (202501010000-202511302358): ");
                if (scanf("%ld", &v_FechaObservacionNum) != 1 || !valida_dato("F", v_FechaObservacionNum)) {
                    while (getchar() != '\n');
                    ImprimeMensaje("Dato invalido");
                    continue;
                }
                break;
            }
        } else if (*opcion == 4) { // búsqueda
            if (v_CodigoEstacion == -1 || v_CodigoSensor == -1 || v_FechaObservacionNum == -1) {
                ImprimeMensaje("\n\tFaltan datos...");
            } else {
                printf("\n\tBuscando estado...\n");

                clock_t start = clock();

                // Escribir datos en memoria compartida
                ptr->CodigoEstacionNuevo_id = v_CodigoEstacion;
                ptr->CodigoSensor_id = v_CodigoSensor;
                ptr->FechaObservacionNum_id = v_FechaObservacionNum;
                ptr->listo = 1;

                // Esperar respuesta del buscador
                while (ptr->listo != 2) {
                    usleep(1000);
                }

                float resultado = ptr->resultado;
                clock_t end = clock();
                double tiempo = (double)(end - start) / CLOCKS_PER_SEC;

                if (resultado != -1) {
                    sprintf((char *)mensaje,
                            "Para estacion: %d sencor: %d fecha: %ld\n"
                            "El Valor Observado es: %f\n"
                            "El tiempo de busqueda fue: %.6f segundos\n",
                            v_CodigoEstacion, v_CodigoSensor, v_FechaObservacionNum, resultado, tiempo);
                } else {
                    sprintf((char *)mensaje,
                            "\tNA.\n"
                            "El tiempo de busqueda fue: %.6f segundos\n",
                            tiempo);
                }
                ImprimeMensaje((char*)mensaje);

                //ptr->listo = 0; // resetear bandera
            }
        }
    }
    free(opcion);
    free(mensaje);
    opcion = NULL;
    mensaje = NULL;
    return 0;
}

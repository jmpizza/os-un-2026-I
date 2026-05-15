#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <termios.h>

#include "shared.h"

#define PORT 8080

Request DataInput={0};
Response DataOutput={};

char ip_servidor[64];

long RecordCount() {
    FILE *index = fopen("data_index.dat", "rb");
    if (!index) {
        perror("Error abriendo indice");
        return 0;
    }
    long file_size;
    fseek(index, 0, SEEK_END);
    file_size = ftell(index);
    long bucket_bytes = TABLE_SIZE * sizeof(Bucket);
    long node_bytes = sizeof(HashNode);
    long num_nodos = (file_size - bucket_bytes) / node_bytes;
    fclose(index);
    return num_nodos;
}

void BorrarPantalla() {
    system("clear");
}

void PresioneCualquierTecla() {
    printf("Presione cualquier tecla para continuar...");
    fflush(stdout);
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
}

void ImprimeMensaje(const char *mensaje) {
    printf("%s\n", mensaje);
    PresioneCualquierTecla();
}

void MenuMain() {
    printf("----------------------------------\n");
    printf("          Bienvenido\n");
    printf("Temperatura Ambiente del Aire\n");
    printf("----------------------------------\n");
    printf("  1.  Busqueda Valor observado por llave Primaria\n");
    printf("  2.  Busqueda Informacion por Numero de Registro\n");
    printf("  3.  Ingresar Nuevo registro\n\n");
    printf("  4.  <Salir> del Programa\n");
}

void Menukey() {
    printf("----------------------------------\n");
    printf("Temperatura Ambiente del Aire\n");
    printf("** BUSQUEDA POR LLAVE PRIMARIA **  \n");
    printf("ESTACION: %d SENSOR: %d FECHA: %ld\n", DataInput.key.CodigoEstacionNuevo, DataInput.key.CodigoSensor, DataInput.key.FechaObservacionNum);
    printf("----------------------------------\n");
    printf("  1.  Ingresar Codigo de Estacion\n");
    printf("  2.  Ingresar Codigo de Sensor\n");
    printf("  3.  Ingresar Codigo de Fecha\n");
    printf("  4.  Buscar Valor observado\n\n");
    printf("  5.  <Volver> a Menu Principal\n");
}

void MenuRec() {
    printf("----------------------------------\n");
    printf("Temperatura Ambiente del Aire\n");
    printf("** BUSQUEDA POR NUMERO DE REGISTRO  **\n");
    printf("Numero REgistro: %ld\n",DataInput.RecordNum);
    printf("----------------------------------\n");
    printf("  1. Ingresar Numero de Registro\n");
    printf("  2. Buscar Informacion\n\n");
    printf("  3. Volver a Menu Principal\n");
}

void MenuCreate() {
    printf("----------------------------------\n");
    printf("Temperatura Ambiente del Aire\n");
    printf("**   INGRESO DE DATOS   **\n");
    printf("ESTACION: %d SENSOR: %d FECHA: %ld Valor Obsevado: %.2f\n", DataInput.key.CodigoEstacionNuevo, DataInput.key.CodigoSensor, DataInput.key.FechaObservacionNum,DataInput.ValorObservado);
    printf("Nombre: %s Departamento; %s, Municipio: %s\n", DataInput.NombreEstacion, DataInput.Departamento, DataInput.Municipio);
    printf("Zona: %s Longitud: %f, Latitud: %f\n", DataInput.ZonaHidrografica, DataInput.Longitud, DataInput.Latitud);
    printf("----------------------------------\n");
    printf("  1. Ingresar Codigo de Estacion\n");
    printf("  2. Ingresar Codigo de Sensor\n");
    printf("  3. Ingresar Valor observado\n");
    printf("  4. Ingresar Nombre de la estación\n");
    printf("  5. Ingresar Departamento\n");
    printf("  6. Ingresar Municipio\n");
    printf("  7. Ingresar Zona hidrografica\n");
    printf("  8. Ingresar Latitud\n");
    printf("  9. Ingresar Longitud\n");
    printf("  10. Crear Registro\n\n");
    printf("  11. Volver a Menu Principal\n");
}

int Muestra_Menu(int opcion) {
    int val_user;
    int result;
    int maxvalue;
    while (true) {
        BorrarPantalla();
        if (opcion==0){
            MenuMain();
            maxvalue = 4;
        }else if (opcion ==1) {
            Menukey();
            maxvalue = 5;
        } else if (opcion ==2) {
            MenuRec();
            maxvalue = 3;
        }else if (opcion == 3) {
            MenuCreate();
            maxvalue = 11;
        }
        printf("\n\tDigite una opcion <1-%d>: ", maxvalue);
        result = scanf("%d", &val_user);
        while (getchar() != '\n');
        if (result != 1 || val_user < 1 || val_user > maxvalue) {
            char tmp[128];
            sprintf(tmp, "\t<Valor equivocado> Digite un valor entre 1 y %d", maxvalue);
            ImprimeMensaje(tmp);
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
        char fechaHora[20];
        time_t t;
        struct tm *tm_info;
        long long fechaHoraMax;
        time(&t);
        tm_info = localtime(&t);
        strftime(fechaHora, sizeof(fechaHora), "%Y%m%d%H%M", tm_info);
        fechaHoraMax = strtoll(fechaHora, NULL, 10);
        return (valor >= 202501010000LL && valor <= fechaHoraMax);
    }else if (tipo[0] == 'R') {
        long TotalRecord = RecordCount();
        return (valor >= 1L && valor <= TotalRecord);
    }else if (tipo[0] == 'V') {
        return (valor >= 0);
    }
    return false;
}

bool valida_Opcion(Request Informacion){
    if (Informacion.op == 1)  {
        if (Informacion.key.CodigoEstacionNuevo == 0 || Informacion.key.CodigoSensor == 0 || Informacion.key.FechaObservacionNum == 0) {
            return false;
        }
    } else if (Informacion.op  == 2)  {
        if (Informacion.RecordNum == 0 ) {
            return false;
        }
    }else if (Informacion.op  == 3)  {
        if (Informacion.key.CodigoEstacionNuevo ==  0 || Informacion.key.CodigoSensor == 0  || Informacion.ValorObservado == 0) {
            return false;
        }
    }
    return true;
}

Response enviarRequest(Request req) {
    Response res = {0};
    int sock;
    struct sockaddr_in serv_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) { perror("Error creando socket"); exit(1); }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    if (inet_pton(AF_INET, ip_servidor, &serv_addr.sin_addr) <= 0) {
        perror("Dirección inválida"); exit(1);
    }
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error en connect"); exit(1);
    }
    if (send(sock, &req, sizeof(req), 0) < 0) {
        perror("Error enviando datos");
    }
    int n = recv(sock, &res, sizeof(res), 0);
    if (n <= 0) {
        printf("No se recibió respuesta del servidor\n");
        res.status = 1;
    }
    close(sock);
    return res;
}

long leerDato(const char *prompt, const char *tipo) {
    long valor;
    while (true) {
        printf("%s", prompt);
        if (scanf("%ld", &valor) != 1 || !valida_dato(tipo, valor)) {
            while (getchar() != '\n');
            ImprimeMensaje("Dato invalido");
            continue;
        }
        break;
    }
    return valor;
}

void leerString(const char *prompt, const char *tipo, char *buffer, int maxLen) {
    char entrada[256];
    while (true) {
        printf("%s \n", prompt);
        while (getchar() != '\n');
        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            ImprimeMensaje("Error de lectura");
            continue;
        }
        entrada[strcspn(entrada, "\n")] = '\0';
        if (strlen(entrada) == 0) {
            strncpy(buffer, "n/a", maxLen);
            buffer[maxLen-1] = '\0';
        } else {
            strncpy(buffer, entrada, maxLen);
            buffer[maxLen-1] = '\0';
        }
        break;
    }
}

float leerFloat(const char *prompt, const char *tipo) {
    float valor = 0.0f;
    char entrada[64];
    while (true) {
        printf("%s", prompt);
        while (getchar() != '\n');
        if (fgets(entrada, sizeof(entrada), stdin) == NULL) {
            ImprimeMensaje("Error de lectura");
            continue;
        }
        entrada[strcspn(entrada, "\n")] = '\0';
        if (strlen(entrada) == 0) {
            return 0.0f;
        }
        if (sscanf(entrada, "%f", &valor) != 1) {
            ImprimeMensaje("Dato invalido");
            continue;
        }
        if (tipo[0] == 'L' && (valor < -90 || valor > 90)) {
            ImprimeMensaje("Dato invalido");
            continue;
        }
        if (tipo[0] == 'K' && (valor < -180 || valor > 180)) {
            ImprimeMensaje("Dato invalido");
            continue;
        }
        break;
    }
    return valor;
}

void MenuOpcion1() {
    memset(&DataInput, 0, sizeof(DataInput));
    DataInput.op = 1;
    int opcion;
    char mensaje[250];
    while (true) {
        opcion = Muestra_Menu(1);
        if (opcion == 5) break;
        else if (opcion == 1)
            DataInput.key.CodigoEstacionNuevo = leerDato("Ingrese ID de Estacion (100-513): ", "E");
        else if (opcion == 2)
            DataInput.key.CodigoSensor = leerDato("Ingrese ID de Sensor (68/71): ", "S");
        else if (opcion == 3)
            DataInput.key.FechaObservacionNum = leerDato("Ingrese fecha (202501010000 - Fecha Actual): ", "F");
        else if (opcion == 4) {
            if (!valida_Opcion(DataInput)) {
                ImprimeMensaje("\n\tFaltan datos...");
            } else {
                printf("\n\tBuscando valor...\n");
                clock_t start = clock();
                Response DataOutput = enviarRequest(DataInput);
                clock_t end = clock();
                double tiempo = (double)(end - start) / CLOCKS_PER_SEC;
                printf("\n\tRespuesta del servidor:\n");
                if (DataOutput.status == 0) {
                    sprintf(mensaje,
                            "Para estacion: %d sensor: %d fecha: %ld\n"
                            "El valor observado es: %.2f\n"
                            "El tiempo de busqueda fue: %.6f segundos\n",
                            DataInput.key.CodigoEstacionNuevo,
                            DataInput.key.CodigoSensor,
                            DataInput.key.FechaObservacionNum,
                            DataOutput.ResInfo.ValorObservado,
                            tiempo);
                } else {
                    sprintf(mensaje,
                            "\tNA.\n"
                            "El tiempo de busqueda fue: %.6f segundos\n",
                            tiempo);
                }
                ImprimeMensaje(mensaje);
            }
        }
    }
}

void MenuOpcion2() {
    memset(&DataInput, 0, sizeof(DataInput));
    DataInput.op = 2;
    int opcion;
    char mensaje[250];
    while (true) {
        opcion = Muestra_Menu(2);
        if (opcion == 3) {
            break;
        } else if (opcion == 1) {
            char prompt[100];
            sprintf(prompt, "Ingrese Numero de registro [1 a  %ld] >: ", RecordCount());
            DataInput.RecordNum = leerDato(prompt, "R");
        } else if (opcion == 2) {
            if (!valida_Opcion(DataInput)) {
                ImprimeMensaje("\n\tFaltan datos...");
            } else {
                printf("\n\tBuscando valor...\n");
                clock_t start = clock();
                Response DataOutput = enviarRequest(DataInput);
                clock_t end = clock();
                double tiempo = (double)(end - start) / CLOCKS_PER_SEC;
                printf("\n\tRespuesta del servidor:\n");
                if (DataOutput.status == 0) {
                    sprintf(mensaje,
                            "Para el registro #: %ld\n"
                            "Corresponde a estacion: %d sensor: %d fecha: %ld\n"
                            "El valor observado es: %.2f C\n"
                            "El tiempo de busqueda fue: %.6f segundos\n",
                            DataInput.RecordNum,
                            DataOutput.ResInfo.key.CodigoEstacionNuevo,
                            DataOutput.ResInfo.key.CodigoSensor,
                            DataOutput.ResInfo.key.FechaObservacionNum,
                            DataOutput.ResInfo.ValorObservado,
                            tiempo);
                } else {
                    sprintf(mensaje,
                            "\tNA.\n"
                            "El tiempo de busqueda fue: %.6f segundos\n",
                            tiempo);
                }
                ImprimeMensaje(mensaje);
            }
        }
    }
}

void MenuOpcion3() {
    memset(&DataInput, 0, sizeof(DataInput));
    DataInput.op = 3;
    int opcion;
    char mensaje[250];
    while (true) {
        opcion = Muestra_Menu(3);
        if (opcion == 11) {
            break;
        } else if (opcion == 1) {
            DataInput.key.CodigoEstacionNuevo = leerDato("Ingrese ID de Estacion (100-513): ", "E");
        } else if (opcion == 2) {
            DataInput.key.CodigoSensor = leerDato("Ingrese ID de Sensor (68/71): ", "S");
        } else if (opcion == 3) {
            while (true) {
                printf("\rIngrese el valor observado: ");
                if (scanf("%f", &DataInput.ValorObservado) != 1 || !(DataInput.ValorObservado >= 0)) {
                    while (getchar() != '\n');
                    ImprimeMensaje("Dato invalido");
                    continue;
                }
                break;
            }
        } else if (opcion == 4) {
            char NombreEstacion[60];
            leerString("Ingrese Nombre de la estación: ", "N", NombreEstacion, 60);
            strcpy(DataInput.NombreEstacion, NombreEstacion);
        } else if (opcion == 5) {
            char Departamento[60];
            leerString("Ingrese Departamento: ", "N", Departamento, 60);
            strcpy(DataInput.Departamento, Departamento);
        } else if (opcion == 6) {
            char Municipio[35];
            leerString("Ingrese Municipio : ", "M", Municipio, 35);
            strcpy(DataInput.Municipio, Municipio);
        } else if (opcion == 7) {
            char ZonaHidrografica[30];
            leerString("Ingrese Zona hidrografica: ", "Z", ZonaHidrografica, 30);
            strcpy(DataInput.ZonaHidrografica, ZonaHidrografica);
        } else if (opcion == 8) {
            DataInput.Latitud = leerFloat("Ingrese Latitud: ", "L");
        } else if (opcion == 9) {
            DataInput.Longitud = leerFloat("Ingrese Longitud: ", "K");
        } else if (opcion == 10) {
            if (!valida_Opcion(DataInput)) {
                ImprimeMensaje("\n\tFaltan datos...");
            } else {
                printf("\n\tCreando registro...\n");
                clock_t start = clock();
                Response DataOutput = enviarRequest(DataInput);
                clock_t end = clock();
                double tiempo = (double)(end - start) / CLOCKS_PER_SEC;
                if (DataOutput.status == 0) {
                    sprintf(mensaje,
                            "Registro creado:\n"
                            "Estacion: %d, Sensor: %d, Fecha: %ld\n"
                            "Valor observado: %.2f\n"
                            "Tiempo de operación: %.6f segundos\n",
                            DataInput.key.CodigoEstacionNuevo,
                            DataInput.key.CodigoSensor,
                            DataInput.key.FechaObservacionNum,
                            DataOutput.ResInfo.ValorObservado,
                            tiempo);
                } else {
                    sprintf(mensaje,
                            "\tNA.\n"
                            "Tiempo de operación: %.6f segundos\n",
                            tiempo);
                }
                ImprimeMensaje(mensaje);
            }
        }
    }
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
    printf("Ingrese la IP del servidor [default: 127.0.0.1]: ");
    if (fgets(ip_servidor, sizeof(ip_servidor), stdin) != NULL) {
        ip_servidor[strcspn(ip_servidor, "\n")] = '\0';
        if (strlen(ip_servidor) == 0) {
            strcpy(ip_servidor, "127.0.0.1");
        }
    } else {
        strcpy(ip_servidor, "127.0.0.1");
    }
    printf("Usando IP del servidor: %s\n", ip_servidor);
    while (true) {
        *opcion = Muestra_Menu(0);
        if (*opcion == 4) {
            printf("\n\tGracias por utilizar el programa\n");
            break;
        } else if (*opcion == 1) {
            MenuOpcion1();
        } else if (*opcion == 2) {
            MenuOpcion2();
        } else if (*opcion == 3) {
            MenuOpcion3();
        }
    }
    free(opcion);
    free(mensaje);
    opcion = NULL;
    mensaje = NULL;
    return 0;
}
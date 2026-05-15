#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>

#define PORT 8080

#include "shared.h"       
#include "hash.h" 

int server_fd;

void manejar_sigint(int sig) {
    printf("\nServidor detenido con señal %d (Ctrl+C)\n", sig);
    close(server_fd);
    exit(0);
}

static bool parse_record_line(const char *raw, Record *r) {
    return sscanf(raw,
        "%ld,%d,%29[^,],%f,%59[^,],%59[^,],%29[^,],%34[^,],%f,%f,%34[^,],%4[^,],%d,%ld",
        &r->CodigoEstacion, &r->CodigoSensor, r->FechaObservacion,
        &r->ValorObservado, r->NombreEstacion, r->Departamento,
        r->Municipio, r->ZonaHidrografica, &r->Latitud, &r->Longitud,
        r->DescripcionSensor, r->UnidadMedida, &r->CodigoEstacionNuevo,
        &r->FechaObservacionNum) == 14;
}

static bool read_record_at_offset(FILE *csv, long offset, Record *r) {
    if (fseek(csv, offset, SEEK_SET) != 0) {
        perror("Error posicionando en CSV");
        return false;
    }
    char raw[256];
    size_t n = fread(raw, 1, sizeof(raw) - 1, csv);
    if (n == 0) return false;
    raw[n] = '\0';

    if (!parse_record_line(raw, r)) {
        fprintf(stderr, "Error parseando línea CSV en offset %ld\n", offset);
        return false;
    }
    return true;
}

static FILE *open_index(const char *indexFile) {
    FILE *idx = fopen(indexFile, "rb");
    if (!idx) {
        perror("Error abriendo índice");
        exit(EXIT_FAILURE);
    }
    return idx;
}

static FILE *open_csv(const char *csvFile) {
    FILE *csv = fopen(csvFile, "r");
    if (!csv) {
        perror("Error abriendo CSV");
        exit(EXIT_FAILURE);
    }
    return csv;
}

Record search(CompositeKey key, const char *csvFile, const char *indexFile) {
    Record r = {0};
    Bucket b;
    HashNode node;
    unsigned int idx = hash_function(key);

    FILE *index = fopen(indexFile, "rb");
    if (!index) {
        perror("Error abriendo índice");
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

        if (node.key.CodigoEstacionNuevo == key.CodigoEstacionNuevo &&
            node.key.CodigoSensor == key.CodigoSensor &&
            node.key.FechaObservacionNum == key.FechaObservacionNum) {

            printf("Offset encontrado : %lld\n", (long long)node.offset);

            if (!read_record_at_offset(csv, node.offset, &r))
                break;

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

Record searchRecordNum(int64_t RecordNum, const char *csvFile, const char *indexFile) {
    Record r = {0};
    HashNode node;

    FILE *index = fopen(indexFile, "rb");
    if (!index) {
        perror("Error abriendo índice");
        return r;
    }
    FILE *csv = fopen(csvFile, "r");
    if (!csv) {
        perror("Error abriendo CSV");
        fclose(index);
        return r;
    }

    long bucket_offset = TABLE_SIZE * sizeof(Bucket);
    long node_offset = bucket_offset + (RecordNum - 1) * sizeof(HashNode);
    printf("offset Nodo: %ld\n", node_offset);

    fseek(index, node_offset, SEEK_SET);
    if (fread(&node, sizeof(HashNode), 1, index) != 1) {
        perror("Error leyendo nodo");
        fclose(index);
        fclose(csv);
        return r;
    }

    if (!read_record_at_offset(csv, node.offset, &r)) {
        fclose(index);
        fclose(csv);
        return (Record){0};
    }

    fclose(index);
    fclose(csv);
    return r;
}

void writeLog(const char *ipClient, Response res) {
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    char fechaHora[20];
    char nombreArchivo[40];
    strftime(fechaHora, sizeof(fechaHora), "%Y%m%dT%H%M%S", tm_info);
    strftime(nombreArchivo, sizeof(nombreArchivo), "log_server_%Y%m%d.log", tm_info);

    const char *Operation;
    if (res.ResInfo.op == 1) {
        Operation = "Busqueda por llave primaria";
    } else if (res.ResInfo.op == 2) {
        Operation = "Busqueda por numero de registro";
    } else if (res.ResInfo.op == 3) {
        Operation = "Ingreso nuevo dato";
    } else {
        Operation = "Operacion desconocida";
    }

    FILE *f = fopen(nombreArchivo, "a");
    if (!f) {
        perror("Error al abrir el archivo de log");
        return;
    }

    fprintf(f,
        "[%s] Cliente [%s] Operacion [%s] LlavePrimaria [Estacion:%d Sensor:%d Fecha:%ld] Status [%s]\n",
        fechaHora,
        ipClient,
        Operation,
        res.ResInfo.key.CodigoEstacionNuevo,
        res.ResInfo.key.CodigoSensor,
        res.ResInfo.key.FechaObservacionNum,
        (res.status == 0 ? "OK" : "Error"));

    fclose(f);
}

static long append_to_csv(const Request *req) {
    FILE *csv = fopen("data.csv", "a");
    if (!csv) {
        perror("Error abriendo el archivo csv");
        exit(EXIT_FAILURE);
    }

    long offset = ftell(csv);

    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char date[30];
    strftime(date, sizeof(date), "%Y %b %d %I:%M:%S %p", t);

    char new_date[15];
    strftime(new_date, sizeof(new_date), "%Y%m%d%H%M", t);
    int64_t new_date_num = atoll(new_date);
    ((Request*)req)->key.FechaObservacionNum = new_date_num;

    fprintf(csv, "%d,%d,%s,%f,%s,%s,%s,%s,%f,%f,%s,%s,%d,%ld\n",
            req->key.CodigoEstacionNuevo,
            req->key.CodigoSensor,
            date,
            req->ValorObservado,
            req->NombreEstacion,
            req->Departamento,
            req->Municipio,
            req->ZonaHidrografica,
            req->Latitud,
            req->Longitud,
            "TEMPERATURA DEL AIRE A 2 m",
            "°C",
            req->key.CodigoEstacionNuevo,
            new_date_num);

    fclose(csv);
    return offset;
}

static void insert_into_index(const char *indexFile, CompositeKey key, long offset) {
    FILE *index = fopen(indexFile, "r+b");
    if (!index) {
        perror("Error abriendo índice para inserción");
        exit(EXIT_FAILURE);
    }
    hash_insert(index, key, offset);
    fclose(index);
}

void writeRecord(Request *req) {
    long offset = append_to_csv(req);
    insert_into_index("data_index.dat", req->key, offset);
}

typedef void (*OperationHandler)(const Request *req, Response *res, const char *ipClient);

static void handle_search_key(const Request *req, Response *res, const char *ipClient) {
    printf("entro opcion 1\n");
    printf("Busqueda por llave: Estacion %d, Sensor %d, Fecha %ld\n",
           req->key.CodigoEstacionNuevo,
           req->key.CodigoSensor,
           req->key.FechaObservacionNum);

    CompositeKey key;
    key.CodigoEstacionNuevo = req->key.CodigoEstacionNuevo;
    key.CodigoSensor = req->key.CodigoSensor;
    key.FechaObservacionNum = req->key.FechaObservacionNum;

    Record r = search(key, "data.csv", "data_index.dat");

    if (r.CodigoEstacion >= 100) {
        printf("Encontro informacion solicitada\n");
        res->ResInfo.ValorObservado = r.ValorObservado;
        res->ResInfo.key.CodigoEstacionNuevo = r.CodigoEstacionNuevo;
        res->ResInfo.key.CodigoSensor = r.CodigoSensor;
        res->ResInfo.key.FechaObservacionNum = r.FechaObservacionNum;
        res->status = 0;
    } else {
        printf("No encontro informacion\n");
        res->ResInfo = *req;
        res->status = 1;
    }
    writeLog(ipClient, *res);
    printf("Logs escritos correctamente.\n");
}


static void handle_search_record(const Request *req, Response *res, const char *ipClient) {
    printf("entro opcion 2\n");
    printf("Busqueda por Numero de Registro: %ld\n", req->RecordNum);

    Record r = searchRecordNum(req->RecordNum, "data.csv", "data_index.dat");
    printf("Despues de buscar con opcion #2\n");

    if (r.CodigoEstacion >= 100) {
        printf("Encontro informacion solicitada\n");
        res->ResInfo.ValorObservado = r.ValorObservado;
        res->ResInfo.key.CodigoEstacionNuevo = r.CodigoEstacionNuevo;
        res->ResInfo.key.CodigoSensor = r.CodigoSensor;
        res->ResInfo.key.FechaObservacionNum = r.FechaObservacionNum;
        res->status = 0;
    } else {
        printf("No encontro informacion\n");
        res->ResInfo = *req;
        res->status = 1;
    }
    writeLog(ipClient, *res);
    printf("Logs escritos correctamente.\n");
}

static void handle_create_record(const Request *req, Response *res, const char *ipClient) {
    printf("entro opcion 3\n");

    writeRecord((Request *)req);

    printf("\nRegistro creado: Estacion %d, Sensor %d, Fecha %ld, Valor %.2f",
           req->key.CodigoEstacionNuevo,
           req->key.CodigoSensor,
           req->key.FechaObservacionNum,
           req->ValorObservado);

    res->ResInfo = *req;
    res->ResInfo.ValorObservado = req->ValorObservado;
    res->status = 0;

    writeLog(ipClient, *res);
}

static void handle_unknown(const Request *req, Response *res, const char *ipClient) {
    (void)req;       
    (void)ipClient;
    printf("Operacion desconocida\n");
    res->status = 1;
}

static const OperationHandler handlers[] = {
    [OP_BUSQUEDA_LLAVE]    = handle_search_key,
    [OP_BUSQUEDA_REGISTRO] = handle_search_record,
    [OP_CREAR_REGISTRO]    = handle_create_record,
};
static const size_t handler_count = sizeof(handlers) / sizeof(handlers[0]);

static void handle_client(int client_fd, struct sockaddr_in client) {
    
    char ip_cliente[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client.sin_addr, ip_cliente, INET_ADDRSTRLEN);
    printf("Cliente conectado desde IP: %s\n", ip_cliente);

    Request req;
    int n = recv(client_fd, &req, sizeof(req), 0);
    if (n <= 0) {
        perror("Error leyendo request");
        close(client_fd);
        return;
    }

    Response res;
    memset(&res, 0, sizeof(res));
    res.ResInfo.op = req.op;
    res.status = 0;

    printf("Operación recibida: %d\n", req.op);

    if (req.op >= 0 && req.op < (int)handler_count && handlers[req.op] != NULL) {
        handlers[req.op](&req, &res, ip_cliente);
    } else {
        handle_unknown(&req, &res, ip_cliente);
    }

    printf("Envia informacion solicitada\n");
    send(client_fd, &res, sizeof(res), 0);
    close(client_fd);
}


static int create_server_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == 0) {
        perror("Error creando socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 32) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }

    return fd;
}

int main() {
    signal(SIGINT, manejar_sigint);

    int server_fd = create_server_socket(PORT);

    printf("Servidor escuchando en puerto %d...\n", PORT);
    printf("Para Terminar proceso del Servidor (Ctrl+C)\n");

    while (true) {
        struct sockaddr_in client;
        socklen_t addrlen = sizeof(client);

        int new_socket = accept(server_fd, (struct sockaddr *)&client, &addrlen);
        if (new_socket < 0) {
            perror("Error en accept");
            exit(EXIT_FAILURE);
        }
        printf("conexion aceptada\n");

        handle_client(new_socket, client);
    }

    close(server_fd);
    return 0;
}
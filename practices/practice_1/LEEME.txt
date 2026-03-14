Sistemas Operativos - Practica 1- Procesos y comunicación entre procesos
========================================================================
Este proyecto contiene dos programas en C: una interfaz básica y un buscador de datos. 
1.  Requisitos
- GCC
- Make
- Sistema operativo Linux (probado en Ubuntu)

2. Compilación
--------------
Ejecutar:
    make

Esto generará dos ejecutables:
    - interfaz  (cliente con menú)
    - buscador  (servidor que busca en Temperatura_6.csv)

3. Ejecución
------------
El cliente y el servidor se comunican mediante memoria compartida,
utilizando estructuras definidas en el archivo shared.h. El servidor escribe
los resultados de la búsqueda en la memoria compartida y el cliente
los lee para mostrarlos en pantalla.

Primero iniciar el servidor:
    ./buscador

El buscador quedará escuchando 

Luego iniciar el cliente:
    ./interfaz


4. Uso
------
En el cliente se visualizaran las siguientes opciones:
    - 1. Ingresar Codigo de Estacion (10-521)
    - 2. Ingresar Codigo de Sensor (68/71)
    - 3. Ingresar Codigo de Fecha (202501010000-202512312358)
    - 4. buscar Valor observado
    - 5. salir del Programa

El servidor responderá con el valor Observado utilizando la estructura .h

5. Archivos importantes
-----------------------
- data.csv          : archivo de datos con informacion de Temperatura Ambiente del Aire.
- Makefile          : script de compilación.
- ui.c  	    : código fuente del cliente.
- hash_search.c.c   : código fuente del buscador.
- shared.h	    : código fuente de estructura para la memoria compartida
- data_index.dat    : archivo binario con los indices.
- hash.c            : código fuente donde esta la funcion hash
- hash.h            : código fuente donde esta la funcion hash
- build_hash.c      : código funete para construir el archivo data_idex.dat

6. Autores 
-----------------------
Laura Natalia Duarte Acero
Juan Manuel Espitia Pizza
Sebastian Steeven Rodriguez Ortiz


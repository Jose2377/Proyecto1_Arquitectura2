FUNCIONES:
WRITE_CACHE (ADDRESS), (VALUE)
-Llenar cache
WRITE_MEM (ADDRESS_MEMORY), (AMOUNT_BYTES_CACHE), (ADDRESS_CACHE), (QOS)
- Escribir en memoria los datos enviados por PE SRC la dirección ADDR // Los datos por escribir vienen del caché del PE SRC
READ_MEM (ADDRESS), (AMOUNT_BYTES), (QOS)
- Leer de memoria un bloque de tamaño SIZE a partir de la dirección ADDR // El PE SRC almacena los datos en su caché
BROADCAST_INVALIDATE (SRC_CACHE) (CACHE_LINE) (QoS)
- Invalida la línea de cache CACHE_LINE de todos los PEs del sistema

MENSAJES:
INV_BACK (SRC_CACHE) (QoS)
- Respuesta a la invalidación de un PE
INV_COMPLETE (PE_DEST) (QoS)
- Respuesta que indica todas las invalidaciones fueron completadas

SINCRONIZACIÓN y respuestas:
READ_RESP (PE_DEST) (DATA) (QoS)
- Datos de la Memoria principal correspondiente es a READ_MEM // La cantidad de bytes debe ser consistente con el mensaje de READ_MEM
WRITE_RESP (PE_DEST) (STATUS) (QoS)
- Respuesta a WRITE_MEM // STATUS es un campo de 1- byte indicando si la escritura fue exitosa: 0x1: OK 0x0: NOT_OK




Ancho de banda: Interconnect

Tráfico: cada PE

Tiempo de ejecución 
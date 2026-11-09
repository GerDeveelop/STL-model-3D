# Importación STL — incremento 0.1

> Estado local 0.3.2 en validación: lectura de pintura compilada y miniz autorizada; Windows bloquea ejecución final con 4551. Evidencia preliminar y límites en [PAINT_IMPORT.md](PAINT_IMPORT.md). Sin revisión visual, push ni CI nueva; última CI publicada 0.2.2. Las aceptaciones 0.3.1 siguientes son históricas.

> Actualización 2026-11-08: fase 0.1 aceptada por el usuario para continuar. Incremento 0.2 (STL/3MF, plano, corte y exportación) y evidencia vigente en [PHASE_02.md](PHASE_02.md). Los resultados fechados anteriores se conservan como historial; no acreditan validación del código nuevo ni una nueva ejecución CI.

Fecha del importador: 2026-11-06. Alcance: ASCII/binario, estadísticas, importación en segundo plano y cancelación. Actualización 2026-11-07: conectado al visor OpenGL; véase VIEWER.md para preparación, controles y validación. Sin reparación.

## Uso

Compilar siguiendo [WINDOWS_SETUP.md](WINDOWS_SETUP.md). Abrir la aplicación con el kit Qt en PATH y seleccionar **Archivo > Abrir STL** o Ctrl+O. También se admite una ruta como argumento del ejecutable. Se muestran formato, triángulos, dimensiones y extremos X/Y/Z de la caja envolvente. Las coordenadas originales no se transforman: **STL no define unidades**.

El progreso es indeterminado. Mientras se importa, Abrir queda deshabilitado y Cancelar disponible. El archivo se lee en un QThread; un stop_token permite cancelar entre triángulos y durante grandes secuencias de espacios ASCII. El modelo previo solo se sustituye tras finalizar correctamente; error o cancelación lo conservan. Al cerrar la ventana se solicita cancelación y se espera al hilo antes de destruirlo. Una lectura del sistema operativo en curso no se interrumpe; no se garantiza una latencia máxima de cancelación.

## Contratos y límites

- `core/mesh/Mesh.h`: vértices double, triángulos sin deduplicación, caja envolvente y conteo diagnóstico de degenerados. Sin dependencia Qt.
- `core/io/StlReader.h`: `readStl(path, stop_token)` y `readStl(istream, bytes, stop_token)`. El stream debe ser binario, posicionable y estar al inicio. Devuelve Mesh o lanza runtime_error; la cancelación lanza ImportCancelled. No hay callbacks UI en el núcleo.
- Tamaño máximo inclusivo: 268435456 bytes (256 MiB). Máximo inclusivo: 2000000 triángulos. La sobrecarga por triángulo depende de la representación y capacidad del vector; estos límites no garantizan memoria disponible. La GUI informa bad_alloc sin sustituir el modelo anterior.
- La lectura por ruta exige archivo regular, consulta longitud antes de leer y comprueba al terminar si cambió la longitud. Solo abre en modo lectura; no modifica el original. No detecta todas las modificaciones concurrentes que conserven tamaño.
- Binario: longitud exacta `84 + 50 * count`, enteros little-endian y coordenadas IEEE754 float32 convertidas a double. Esta identificación prevalece incluso con encabezado `solid`. Se rechazan datos truncados, conteos inconsistentes y bytes finales adicionales. Los dos bytes de atributos se conservan solo en el origen y no se interpretan como color.
- Si no coincide la longitud binaria, se exige gramática ASCII completa: un solo `solid`, facetas con normal/outer loop/tres vertex/endloop/endfacet y `endsolid`. Palabras clave en minúsculas, nombres hasta 4096 caracteres por línea, tokens hasta 128 caracteres. Los nombres de apertura/cierre no necesitan coincidir. Se tolera espacio final; no múltiples sólidos concatenados ni contenido posterior. No se admite BOM UTF-8.
- Números ASCII independientes de la configuración regional, con punto decimal y exponentes. Se admiten signos simples; se rechazan signos dobles, valores no finitos, fuera de rango o dimensiones no representables como double finito.
- Las normales suministradas deben ser finitas pero se ignoran; no se alteran el orden ni las coordenadas de los vértices.
- Se conservan mallas abiertas y triángulos degenerados. Un diagnóstico aproximado marca degeneración si el producto cruzado normalizado tiene magnitud menor o igual a `1e-12 * |u| * |v|`, o todas las coordenadas son cero. No equivale a una reparación ni a una comprobación de topología, cierre o autointersecciones.
- Estadísticas UI con 12 dígitos significativos; almacenamiento double. No se aplica tolerancia geométrica ni conversión de escala a las dimensiones. Las comparaciones de cubos verificadas dieron valores exactos representables.

## Inspección sin interfaz

El mismo ejecutable permite automatización local con JSON por stdout y salida 0 para éxito, 1 para error:

```powershell
& .\build\windows-x64-release\STLCutStudioAI.exe --inspect 'C:\ruta\modelo.stl'
```

`--inspect-stdin` recibe bytes por stdin, incluyendo STL binario; usar una herramienta que preserve bytes (por ejemplo subprocess de Python), no una tubería de texto PowerShell para datos binarios. Este modo almacena la entrada acotada en memoria antes de analizarla; el lector por archivo no necesita esa copia completa. Ambos modos usan QCoreApplication sin crear una GUI, aunque necesitan las DLL Qt del kit en PATH.

JSON de éxito: `ok`, `format`, `triangles`, `degenerate_triangles`, `min`, `max`, `dimensions`, `units` (`unspecified`). Error: `ok: false`, `error`. No requiere API keys, red ni variables nuevas: solo las rutas Qt documentadas.

## Validación realizada y límites

Build Release x64 satisfactorio, sin warnings C++ detectados en `build/import-build.log`. Entorno: MSVC 19.51.36252.0, SDK 10.0.26100.0, Qt 6.8.3, CMake 4.3.1-msvc1 y Ninja 1.13.2. Las comprobaciones se ejecutaron en línea, generando entradas en memoria y pasando bytes al ejecutable mediante subprocess. No se escribieron archivos STL ni fuentes de pruebas.

- 18 casos iniciales: cubos ASCII/binario, binario con `solid`, vacío, truncado, conteo inconsistente, bytes finales, NaN ASCII/binario, cierre ausente, cero triángulos ASCII/binario, basura ASCII final, malla abierta, degenerado, sintaxis inválida, número fuera de rango y token excesivo. Cubo: 12 triángulos, mínimo [0,0,0], máximo/dimensiones [10,20,30]. Resultados: `build/import-validation.json`.
- 10 casos posteriores al ajuste final de signos y cancelación: signo doble, signo positivo/exponente, desbordamiento de dimensiones, 10k/100k/1M/2M triángulos, rechazo de 2000001 triángulos, más de 256 MiB y ruta Unicode inexistente. Todos satisfactorios. Resultados: `build/import-boundaries.json`. Tiempos completos de proceso/entrada en memoria observados: 0.023/0.037/0.172/0.332 s para 10k/100k/1M/2M; no son benchmarks de disco ni garantías de rendimiento y no se midió RAM máxima.
- GUI oculta con ruta inexistente: accesibilidad de Windows encontró el mensaje de error y conservación del modelo anterior; ventana operativa y cierre con salida 0. Evidencia: `build/import-ui-elements.json`, stderr vacío en `build/import-ui-stderr.log`.

Pendiente: abrir un STL real con ruta Unicode, verificar visualmente estadísticas, cancelar una importación extensa y conservar un modelo previamente cargado ante error/cancelación. La comprobación GUI de error partía de una escena vacía; no demuestra conservación de un modelo real. No se ejecutaron fixtures de archivo por la instrucción de no crear archivos de pruebas. El visor se implementó y verificó mediante capturas en el incremento del 2026-11-07, con entrada por memoria; detalles en VIEWER.md. La CI está verificada. El cierre de fase 0.1 conserva sus pendientes funcionales, sin exigir segundo PC. El instalador .exe y su validación en VM Windows limpia corresponden a la entrega final de fase 1.0.

## Entrada unificada STL/3MF en 0.2

La interfaz ahora ofrece Abrir STL / 3MF; ruta y stdin usan MeshReader para detectar los bytes. El contrato STL permanece sin conversión de escala. JSON añade unidades, advertencias y volumen firmado (sin certificar cierre). Apertura Unicode y fuente intacta fueron verificadas en la sesión anterior; conservación ante archivo inválido fue confirmada por el usuario. Nuevos contratos 3MF y evidencia actual en PHASE_02.md.

# Requisitos de fase 0.1

Fecha: 2026-11-07. Estado: importador y visor compilados Release con Qt 6.8.3. Malla cargada por memoria en GUI, estadísticas y renderizado comprobados, controles verificados por capturas. Cancelar/Cerrar esperando datos de tubería funcionan. Archivo real Unicode, cancelación durante preparación extensa e interacción manual pendientes. Véase VIEWER.md. Véanse WINDOWS_SETUP.md y CHANGELOG.md. No se crean carpetas ni archivos de pruebas por instrucción del usuario.

## Alcance

Aplicación local Windows 10/11 x64, C++20, Qt 6 y CMake. Usuario principal: persona que abre un STL y revisa su geometría antes de futuras operaciones de corte.

| ID | Requisito | Aceptación prevista |
|---|---|---|
| F01 | Abrir STL ASCII y binario mediante Archivo > Abrir | Un cubo de referencia conserva sus 12 triángulos en ambos formatos |
| F02 | Mostrar cantidad de triángulos, dimensiones y caja envolvente | Valores coinciden con el fixture y su tolerancia numérica documentada |
| F03 | Mostrar la malla y permitir órbita, zoom, paneo y encuadre | Verificación visual con registro de equipo y evidencia |
| F04 | Mostrar ejes y cuadrícula | Visibles y coherentes con la orientación documentada |
| F05 | Informar errores de importación sin cerrar la aplicación | Archivos vacíos, truncados y malformados producen un mensaje comprensible |
| F06 | Mantener la interfaz operativa durante importaciones extensas | Se puede interactuar con la ventana; umbral y mecanismo de cancelación por definir |
| F07 | Preservar el archivo original | La apertura no modifica el archivo fuente |
| B01 | Compilar Release x64 de forma reproducible | Mismo preset y dependencias fijadas en dos equipos y CI Windows |
| B02 | Distribuir un paquete inicial con dependencias de ejecución | Abre e importa STL en otro Windows compatible sin IDE ni gestores de desarrollo |

## Validación de entrada: decisiones del incremento de importación

- Definir tamaño máximo de archivo y número máximo de triángulos, con rechazo previo a asignaciones excesivas.
- Rechazar coordenadas no finitas y detectar conteos o longitudes inconsistentes en STL binario.
- Definir tratamiento de triángulos degenerados, normales incorrectas y datos adicionales al final del archivo.
- Documentar cómo distinguir STL ASCII y binario cuando el encabezado binario comienza con `solid`.
- STL no expresa unidades: acordar unidad asumida y cómo informarla al usuario; no inferir escala silenciosamente.
- Definir tolerancias numéricas, orientación de ejes y precisión de las estadísticas.
- Establecer objetivos de tiempo y memoria después de medir fixtures de 10k, 100k y 1M triángulos en hardware identificado.

Las decisiones de la lista anterior quedan resueltas para este incremento en STL_IMPORT.md: 256 MiB y 2M triángulos, rechazo de datos inválidos, normales finitas ignoradas, conservación de degenerados, binario por longitud exacta y unidades no especificadas. Las comparaciones de cubos y límites se comprobaron en memoria; aceptación GUI completa y medición de memoria siguen pendientes. La lista describe los temas de diseño y el contrato enlazado fija su comportamiento actual.

## Fuera de esta fase

Corte, exportación, conectores, reparación automática, proyectos persistentes, IA, Python, herramientas paramétricas, instalador completo y actualizador. El paquete inicial verifica el runtime; no equivale al instalador final.

## Entorno, configuración y secretos

MSVC x64, Windows SDK, CMake, Ninja, Qt y gestor de dependencias requieren inventario con versión exacta, origen y licencia. El inventario local y Qt Base 6.8.3 instalado están documentados en WINDOWS_SETUP.md; segundo equipo, CI y distribución pendientes. No existe aún un `requirements.txt`, manifiesto vcpkg ni lock de Python; no se necesitan dependencias Python en esta fase.

No se requieren API keys ni endpoints externos para importar y visualizar STL localmente. Si una fase posterior incorpora servicios, registrar nombre de variable, finalidad, obligatoriedad y almacenamiento seguro, sin valores reales. La base usa STL_QT_ROOT para localizar el kit Qt: ruta local no secreta documentada en WINDOWS_SETUP.md.

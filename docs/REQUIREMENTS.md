# Requisitos de fase 0.1

Fecha: 2026-11-05. Estado: definición documental; implementación pendiente de aprobación.

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

## Validación de entrada pendiente de concretar

- Definir tamaño máximo de archivo y número máximo de triángulos, con rechazo previo a asignaciones excesivas.
- Rechazar coordenadas no finitas y detectar conteos o longitudes inconsistentes en STL binario.
- Definir tratamiento de triángulos degenerados, normales incorrectas y datos adicionales al final del archivo.
- Documentar cómo distinguir STL ASCII y binario cuando el encabezado binario comienza con `solid`.
- STL no expresa unidades: acordar unidad asumida y cómo informarla al usuario; no inferir escala silenciosamente.
- Definir tolerancias numéricas, orientación de ejes y precisión de las estadísticas.
- Establecer objetivos de tiempo y memoria después de medir fixtures de 10k, 100k y 1M triángulos en hardware identificado.

Estas decisiones deben quedar resueltas antes de cerrar sus pruebas de aceptación; no son funcionalidades verificadas.

## Fuera de esta fase

Corte, exportación, conectores, reparación automática, proyectos persistentes, IA, Python, herramientas paramétricas, instalador completo y actualizador. El paquete inicial verifica el runtime; no equivale al instalador final.

## Entorno, configuración y secretos

MSVC x64, Windows SDK, CMake, Ninja, Qt y gestor de dependencias requieren inventario con versión exacta, origen y licencia. Ese inventario está pendiente de inspección del equipo y selección técnica. No existe aún un `requirements.txt`, manifiesto vcpkg ni lock de Python; no se necesitan dependencias Python en esta fase.

No se requieren API keys ni endpoints externos para importar y visualizar STL localmente. Si una fase posterior incorpora servicios, registrar nombre de variable, finalidad, obligatoriedad y almacenamiento seguro, sin valores reales. No hay variables de entorno propias aprobadas todavía.

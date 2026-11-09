# Requisitos de fase 0.1

> Estado 0.3.2 publicado y validado en CI Windows: ejecución 7, commit d2a219a, build/paquete y 32 casos generales + 36 de pintura correctos. El bloqueo local 4551 no se ha cambiado; sin revisión visual. Insertos/partición volumétrica pendientes. Evidencia en [CI.md](CI.md); los resultados anteriores conservan su contexto histórico.

> Actualización 2026-11-08: fase 0.1 aceptada por el usuario para continuar. Incremento 0.2 (STL/3MF, plano, corte y exportación) y evidencia vigente en [PHASE_02.md](PHASE_02.md). Los resultados fechados anteriores se conservan como historial; no acreditan validación del código nuevo ni una nueva ejecución CI.

Fecha: 2026-11-07. Estado: importador y visor compilados Release con Qt 6.8.3. Malla cargada por memoria en GUI, estadísticas y renderizado comprobados, controles verificados por capturas. Cancelar/Cerrar esperando datos de tubería funcionan. Archivo real Unicode, cancelación durante preparación extensa e interacción manual pendientes. Véase VIEWER.md. Véanse WINDOWS_SETUP.md y CHANGELOG.md. No se crean carpetas ni archivos de pruebas por instrucción del usuario.

## Alcance

Aplicación local Windows 10/11 x64, C++20, Qt 6 y CMake. Usuario principal: persona que abre un STL y revisa su geometría antes de futuras operaciones de corte.

Decisión vigente: un equipo de desarrollo y CI; se elimina el segundo PC físico como requisito. La fase 0.1 entrega un ZIP portátil provisional. El instalador `.exe` y su validación en VM Windows limpia son obligatorios al finalizar el programa completo, en fase 1.0.

| ID | Requisito | Aceptación prevista |
|---|---|---|
| F01 | Abrir STL ASCII y binario mediante Archivo > Abrir | Un cubo de referencia conserva sus 12 triángulos en ambos formatos |
| F02 | Mostrar cantidad de triángulos, dimensiones y caja envolvente | Valores coinciden con el fixture y su tolerancia numérica documentada |
| F03 | Mostrar la malla y permitir órbita, zoom, paneo y encuadre | Verificación visual con registro de equipo y evidencia |
| F04 | Mostrar ejes y cuadrícula | Visibles y coherentes con la orientación documentada |
| F05 | Informar errores de importación sin cerrar la aplicación | Archivos vacíos, truncados y malformados producen un mensaje comprensible |
| F06 | Mantener la interfaz operativa durante importaciones extensas | Se puede interactuar con la ventana; umbral y mecanismo de cancelación por definir |
| F07 | Preservar el archivo original | La apertura no modifica el archivo fuente |
| B01 | Compilar Release x64 de forma reproducible | Mismo preset y dependencias fijadas en el equipo de desarrollo y CI Windows; versiones y diferencias de toolchain documentadas |
| B02 | Generar un ZIP portátil provisional con dependencias de ejecución | Arranca e importa STL con DLL/plugins/runtime del paquete y PATH mínimo; evidencia local y comprobaciones aplicables en CI |

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

## Entrega final obligatoria de fase 1.0

- `STLCutStudioAI-Setup-<version>-win-x64.exe` instala la aplicación y las dependencias necesarias.
- Tras instalar, el usuario abre `STLCutStudioAI.exe` sin instalar Qt, Python, compiladores ni gestores de desarrollo manualmente.
- Validar en una VM Windows limpia instalación, apertura, funciones incluidas y desinstalación, registrando resultados. Esta validación final todavía no se ha realizado.
- El segundo PC físico no es criterio de aceptación. El instalador todavía no está implementado; esta actualización no autoriza implementarlo ni declara terminado el programa.

## Entorno, configuración y secretos

MSVC x64, Windows SDK, CMake, Ninja, Qt y gestor de dependencias requieren inventario con versión exacta, origen y licencia. El inventario local y Qt Base 6.8.3 instalado están documentados en WINDOWS_SETUP.md; CI verificada y entrega final/condiciones de distribución pendientes. No se exige segundo PC físico. No existe aún un `requirements.txt`, manifiesto vcpkg ni lock de Python; no se necesitan dependencias Python en esta fase.

No se requieren API keys ni endpoints externos para importar y visualizar STL localmente. Si una fase posterior incorpora servicios, registrar nombre de variable, finalidad, obligatoriedad y almacenamiento seguro, sin valores reales. La base usa STL_QT_ROOT para localizar el kit Qt: ruta local no secreta documentada en WINDOWS_SETUP.md.

## Requisitos añadidos para fase 0.2

| ID | Requisito | Aceptación |
|---|---|---|
| F08 | Abrir 3MF de mallas/componentes con unidades y transformaciones | Seis unidades, instancias y reflexión verificadas; advertir pérdida de materiales/ajustes |
| F09 | Plano por normal y posición, con previsualización | Plano coherente con coordenadas del original y piezas |
| F10 | Dos piezas cerradas para geometría admitida | Aristas orientadas, volumen conservado y errores explícitos para casos fuera del contrato |
| F11 | Exportar pieza seleccionada a STL binario | Reimportar bytes antes de guardar, verificar precisión/conteo/volumen y conservar fuente |
| F12 | Cortes encadenados y procedencia | Sustituir solo la pieza seleccionada, conservar hermanas y original de referencia; IDs estables |
| F13 | Deshacer/Rehacer durante la sesión | Restaurar piezas, selección y plano; corte nuevo descarta Rehacer; fallo/cancelación conserva historial; 32 pasos, 256 piezas, 1 GiB contabilizado según HISTORY.md |

El motor vigente desde 0.2.1 admite contornos con huecos y casos coplanares comprobados mediante Manifold 3.5.4. Se mantiene la exigencia de geometría válida y límites de recursos, sin tope independiente de 4096 vértices; véase MANIFOLD_CUT.md. No se promete reparar mallas ni conservar colores/configuración del laminador. Véase PHASE_02.md.

## Alcance aprobado de fase 0.3

Conectores pin/cavidad con parámetros, integración transaccional con Deshacer/Rehacer y separación automática de regiones por color/material 3MF con conectores entre vecinas. Distinguir sólidos existentes de pintura superficial: para esta última el usuario eligió ofrecer ambos modos: insertos de profundidad configurable y partición de todo el volumen. Requisitos y aceptación prevista en [PHASE_03.md](PHASE_03.md). No se declaran funciones implementadas.

Primer incremento implementado: pin/cavidad al cortar, validación de guardas y volumen, parámetros nominales, undo/redo y errores transaccionales. No implementa todavía separación por color; uso y evidencia en PHASE_03.md.

Implementado en 0.3.1: materiales/objetos estándar, separación de regiones cerradas y transacción múltiple de historial; conectores entre dos regiones con contacto plano alineado a ejes. Pintura de fabricante, insertos y partición volumétrica permanecen pendientes. No se interpreta un conjunto abierto de triángulos como un sólido. Límites en COLOR_IMPORT.md.

Construido para 0.3.2: conservar subdivisiones de pintura como regiones superficiales, paleta e herencia de filamentos; reportar asignaciones no resueltas sin inventar colores. La malla fuente no cambia. El binario final aún no tiene aceptación dinámica por bloqueo del sistema; no se implementan ni declaran listos los sólidos de insertos/partición. PAINT_IMPORT.md.

# Visor 3D — fase 0.1

> Estado vigente 0.2.1: Manifold 3.5.4 sustituye el motor propio; cavidades/casos coplanares y contornos >4096 comprobados. Contrato y evidencia en [MANIFOLD_CUT.md](MANIFOLD_CUT.md). Las secciones fechadas anteriores son históricas.

> Actualización 2026-11-08: fase 0.1 aceptada por el usuario para continuar. Incremento 0.2 (STL/3MF, plano, corte y exportación) y evidencia vigente en [PHASE_02.md](PHASE_02.md). Los resultados fechados anteriores se conservan como historial; no acreditan validación del código nuevo ni una nueva ejecución CI.

Fecha: 2026-11-07. Incremento autorizado: visualizar la malla STL con órbita, zoom, paneo, encuadre, ejes y cuadrícula. Compilado y verificado localmente; la fase completa sigue abierta.

## Uso

Compilar y ejecutar siguiendo [WINDOWS_SETUP.md](WINDOWS_SETUP.md). Abrir un STL desde Archivo > Abrir STL, Ctrl+O o pasando su ruta al ejecutable. La malla aparece a la izquierda y las estadísticas a la derecha.

| Control | Acción |
|---|---|
| Arrastrar con botón izquierdo | Órbita alrededor del objetivo de cámara |
| Arrastrar con botón derecho o central | Paneo en el plano de la cámara |
| Rueda | Acercar/alejar |
| F / Vista > Encuadrar | Centrar y ajustar distancia conservando la orientación |
| Inicio/Home / Restablecer cámara | Volver a orientación inicial y encuadrar |
| Cuadrícula / Ejes | Alternar las guías |
| Copiar imagen / Ctrl+Shift+C | Copiar el framebuffer del visor al portapapeles |

Z apunta hacia arriba. X es rojo, Y verde y Z azul; la cuadrícula ocupa el plano XY a la altura mínima del modelo. Un indicador en la esquina mantiene visible la orientación aunque la malla oculte los ejes de la escena. Estos ejes son guías de presentación, no una medida del origen absoluto del archivo. La cuadrícula se escala con la vista normalizada; no expresa milímetros ni otra unidad física.

El renderizado es sólido, con iluminación plana por triángulo y ambas caras visibles. No se reparan normales, topología ni triángulos degenerados. Los controles modifican únicamente la cámara, no la malla importada. Copiar imagen requiere un contexto OpenGL válido y reemplaza la imagen del portapapeles; no modifica el STL.

## Arquitectura y dependencias

Se utiliza QOpenGLWidget con Qt OpenGL/OpenGLWidgets **6.8.3 exacto**, ya incluidos en el kit Qt Base instalado. No se descargaron nuevas bibliotecas ni se incorporó VTK. La alternativa se eligió para implementar este visor acotado con el entorno existente; no decide el motor de cortes de fases posteriores. Referencia: [QOpenGLWidget](https://doc.qt.io/qt-6/qopenglwidget.html). Condiciones de licencia y distribución en [THIRD_PARTY_NOTICES.md](../THIRD_PARTY_NOTICES.md).

- `apps/desktop/MeshView.h/.cpp`: adaptador de presentación, buffers de vértices, shaders, cámara e interacción. El núcleo `stlcut_core` continúa sin Qt/OpenGL.
- `prepareViewMesh`: copia cada triángulo a tres vértices float32 con normal de cara; usa midpoint y escala por la dimensión máxima para centrar/normalizar en coordenadas acotadas. Los datos originales double y sus estadísticas no cambian.
- La copia de presentación se prepara en el mismo QThread de importación, con comprobación de stop_token por triángulo. Solo se entrega a la UI cuando lectura y preparación completan correctamente.
- La subida a la GPU ocurre en paintGL con el contexto activo. No se realiza desde el hilo del lector. Usa buffers y shaders OpenGL 3.3 core; recursos liberados con contexto activo, también al destruir/recrear contexto.
- La cámara utiliza perspectiva de 45 grados, encuadre según esfera envolvente y relación de aspecto. Elevación acotada a ±89 grados, distancia entre 1.05 y 100 radios durante zoom y paneo acotado en el espacio normalizado. F y Home permiten recuperar la vista.
- Los errores de contexto, shaders o asignación gráfica se muestran en el panel. El lector y las estadísticas siguen disponibles; una importación puede completar aunque no haya contexto o memoria gráfica para mostrarla. No se garantiza compatibilidad con cualquier GPU/controlador.

La copia de presentación usa 72 bytes por triángulo (3 vértices × 6 float32) además de la malla original y la memoria GPU. A 2 millones de triángulos son aproximadamente 144 MB por copia de presentación, más la representación original y buffers del controlador. El límite de entrada no garantiza memoria gráfica disponible. No hay simplificación automática de malla ni rendimiento máximo prometido; la asignación/subida GPU puede producir una pausa en la UI.

## Entrada por memoria

`STLCutStudioAI.exe --view-stdin` permite visualizar STL ASCII/binario recibido por una tubería de bytes. A diferencia de `--inspect-stdin`, abre el visor. La GUI permanece operativa mientras espera datos; Cancelar/Cerrar comprueban stop_token mediante sondeo de la tubería. El productor debe finalizar la entrada (EOF) para iniciar el análisis. La entrada se almacena en memoria con el límite de 256 MiB del lector. Se rechazan consola interactiva y entradas que no sean una tubería en este modo.

Se utiliza PeekNamedPipe/ReadFile de Windows; no hay dependencias, API keys ni variables nuevas. Para binarios, utilizar un productor que preserve bytes (por ejemplo subprocess de Python), no una tubería de texto PowerShell. Los modos existentes --inspect y --inspect-stdin conservan su JSON y códigos de salida.

## Evidencia local: 2026-11-07

Entorno: Windows x64, Build Tools 18.8.2, MSVC 19.51.36252.0, SDK 10.0.26100.0, CMake 4.3.1-msvc1, Ninja 1.13.2 y Qt 6.8.3. Contexto real: **OpenGL 3.3.0 NVIDIA 610.88, NVIDIA GeForce RTX 3060 Ti/PCIe/SSE2**.

- `scripts/build-windows.ps1`: Release correcto, sin warnings C++ detectados. Log: `build/viewer-build.log`.
- Malla binaria generada en memoria con encabezado solid, 12 triángulos, caja [0,0,0]–[10,20,30]. Importación GUI y estadísticas verificadas por accesibilidad. Framebuffer de 911×720 inspeccionado visualmente: malla iluminada, cuadrícula y orientación visibles. No se escribieron archivos STL ni fuentes de pruebas.
- Capturas de cada control: `build/viewer-default.png`, `viewer-no-grid.png`, `viewer-no-guides.png`, `viewer-orbit.png`, `viewer-zoom.png`, `viewer-pan.png`, `viewer-fit.png`, `viewer-reset.png`. Se enviaron eventos solo a la ventana oculta y se usaron las acciones accesibles de su barra; no se movió el cursor global.
- Comparación por muestreo cada 3 píxeles: cuadrícula, ejes, órbita, zoom, paneo y encuadre producen cambios. Restablecer cámara devuelve exactamente los píxeles muestreados de la vista inicial. Informe: `build/viewer-image-comparisons.json`. Es una comprobación acotada de imagen, no una prueba de todas las posiciones posibles.
- Cancelar con productor sin enviar datos/EOF y Cerrar en esa misma condición funcionaron; ambos procesos cerraron con código 0. Inspección CLI de cubo, vacío y truncado conservó resultados/códigos esperados. Informe: `build/viewer-validation.json`.
- Metadatos de interfaz y estado gráfico: `build/viewer-ui-elements.json`, `viewer-cancel-elements.json`; logs stderr vacíos. Los primeros intentos de automatización no localizaban la ventana oculta o trataban controles con estado como botones simples; se corrigió la comprobación usando HWND y patrones de accesibilidad apropiados.

Actualización de carga: se midieron 10k/100k/1M/2M, completando recepción/análisis, preparación y envío GPU. Se observó la malla de 10k en ventana visible y cancelación durante recepción de una entrada de 2M, con cierre normal 0. Tiempos, memoria, evidencia, intentos descartados y límites en [PERFORMANCE.md](PERFORMANCE.md). En la sesión posterior con escritorio liberado se verificaron directamente órbita, zoom, F y Home. El usuario realizó el paneo derecho; su desplazamiento no se corroboró visualmente de forma independiente. Detalle en PERFORMANCE.md.

Pendientes: corroboración visual independiente del paneo asistido, apertura real Unicode desde diálogo, retención de malla previa, cancelación durante parser/preparación extensa, VRAM física, hardware sin OpenGL compatible y entrega final .exe/VM Windows limpia en fase 1.0. No se exige segundo PC físico; CI y ZIP provisional verificados, con evidencia en CI.md y BUILD_AND_RELEASE.md. Informes/capturas en build/, excluido de Git; sin archivos de pruebas propios.

## Plano y piezas en 0.2

El visor acepta el adaptador de cualquier malla STL/3MF. Mostrar plano y normal + dibuja un contorno amarillo y un segmento hacia el lado positivo. Seleccionar Original/Parte positiva/Parte negativa cambia la malla visible y su encuadre; la ubicación del plano se adapta al centro/escala de la pieza. Revisión visible en NVIDIA RTX 3060 Ti correcta para cubo 3MF de 10 mm: ambas piezas de 10×10×5 mm, 20 triángulos cada una. Guía completa en PHASE_02.md.

## Revisión visible del motor 0.2.1

En el paquete actual se observaron ambas piezas de un tubo cuadrado, con hueco pasante conservado y nueva superficie de corte anular visible en el lado negativo. Cada parte: 4×4×1, 32 triángulos y volumen 12, original 24. Un plano tangente conservó la pieza anterior con aviso correcto. Cancelación por Ctrl+. sobre 1 929 612 triángulos comprobada. No se implementaron nuevos controles de cámara ni historial de cortes. Evidencia en MANIFOLD_CUT.md.

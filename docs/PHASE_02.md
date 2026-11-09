# Fase 0.2 — Importación 3MF, corte por plano y exportación STL

Fecha: 2026-11-08. Alcance aprobado por el usuario: añadir 3MF junto a STL y continuar corte/exportación. Estado vigente 0.2.2: cortes encadenados y Deshacer/Rehacer de sesión incorporados según [HISTORY.md](HISTORY.md). Motor Manifold 3.5.4 compilado y validado; cavidades y casos coplanares ya están admitidos dentro del contrato de [MANIFOLD_CUT.md](MANIFOLD_CUT.md). Se conservan abajo las evidencias del primer incremento 0.2.0. No equivale a cerrar toda la fase ni a entregar el instalador final.

## Uso

1. Abrir **Archivo > Abrir STL / 3MF…**, Ctrl+O o pasar una ruta al ejecutable. También se detecta el formato de los bytes de entrada por `--inspect-stdin` y `--view-stdin`.
2. Configurar normal X/Y/Z y posición `d` (controles de ±1e12 y ocho decimales). El plano cumple `normal · posición = d`; para un punto P, usar `d = normal · P`. La normal no puede ser nula. Para un corte horizontal usar `(0,0,1)` y la altura Z deseada. Al abrir se centra este plano en Z, dentro del rango de los controles.
3. Activar **Mostrar plano y normal +**. El contorno amarillo representa el plano; el segmento indica el lado positivo. Se muestra en las mismas coordenadas del modelo, aunque cada pieza se encuadra por separado.
4. **Cortar pieza seleccionada** sustituye la pieza activa por ambos lados y conserva las demás. Repetir sobre cualquiera de las piezas activas. Ctrl+Z/Ctrl+Y deshacen/rehacen cortes, selección y plano. El original queda como referencia visible/exportable; detalles y límites en [HISTORY.md](HISTORY.md).
5. **Exportar pieza visible a STL…** guarda únicamente la selección actual en STL binario. Elegir un destino distinto del origen. Exportar cada lado por separado. STL no conserva unidades, materiales ni colores; al importar 3MF sus coordenadas se convierten a mm.

Importación, corte, preparación y exportación trabajan en QThread. Ctrl+. solicita cancelación cooperativa; un fallo/cancelación de corte conserva el original y el resultado anterior. Guardado mediante QSaveFile sin escritura directa alternativa: solo reemplaza el destino después de completar validación y escritura. Si se solicita cancelar después del commit, el archivo ya puede haberse guardado y la aplicación informa éxito.

## Contrato 3MF

- lib3mf **2.5.0**, SDK oficial, API C++ enlazada mediante import library y DLL x64. Núcleo independiente de Qt. Bootstrap explícito: `./scripts/bootstrap-lib3mf.ps1`.
- SDK: `https://github.com/3MFConsortium/lib3mf/releases/download/v2.5.0/lib3mf_sdk_v2.5.0.zip`; 6 871 418 bytes; SHA-256 `d04290d1309f775501d21c50858614cdf75c36fa0cbbfad8706e03e3b66b37d9`. Bootstrap verifica el archivo y los tres headers, import library, DLL y licencia instalados contra el ZIP.
- Recorre exclusivamente los elementos de build y sus componentes, componiendo transformaciones. Instancias repetidas se incluyen; una reflexión invierte el orden de vértices para conservar orientación. Convierte micron/mm/cm/inch/foot/meter a mm. No realiza unión booleana de objetos solapados.
- Solo geometría de mallas/componentes. Colores, texturas, materiales y configuración del laminador no se conservan; aviso visible y en JSON. Beam lattice y objetos no representables como malla se rechazan. No se declara compatibilidad universal con extensiones 3MF.
- El lector SDK usa modo no estricto para admitir metadatos de fabricantes. Se examinan **todas** sus advertencias; solo se toleran `0x80B2` (metadatos desconocidos) y `0x80A7` (atributos de namespace desconocidos), notificándolas una vez por código. Las demás, incluidas extensiones obligatorias no soportadas, producen error. Referencia de códigos: [NMR_ErrorConst.h v2.5.0](https://github.com/3MFConsortium/lib3mf/blob/v2.5.0/Include/Common/NMR_ErrorConst.h).
- Límites: 256 MiB de archivo, 256 MiB descomprimidos declarados, 4096 entradas ZIP, 2 millones de triángulos expandidos, 6 millones de vértices por objeto, 100 000 visitas de objetos y profundidad 64. Se detectan ciclos. No se extrae contenido al disco. ZIP64, cifrado, multivolumen y compresión distinta de stored/deflate se rechazan.
- Los límites ZIP se revisan antes de descomprimir mediante el directorio central; lib3mf/libzip realizan la lectura posterior. No constituyen una cuota rígida de RAM del proceso. Cancelación de SDK mediante callback y comprobaciones durante expansión; no se promete latencia máxima para cada operación interna del SDK.

## Contrato geométrico (motor incorporado en 0.2.1)

El motor actual es Manifold 3.5.4. Admite cavidades/contornos anidados y coincidencias con caras/aristas, sin tope independiente de 4096 vértices. Exige malla cerrada/manifold válida, verifica cierre y volumen y mantiene límites de 2 millones de triángulos/vértices y presupuesto de buffers. Un plano tangente exterior no produce dos piezas de volumen positivo. La cancelación puede demorarse durante el cálculo interno. Contrato, licencia, tolerancias y evidencia nueva en [MANIFOLD_CUT.md](MANIFOLD_CUT.md).

El motor propio del primer incremento se sustituyó; sus restricciones de huecos/coplanaridad/4096 ya no describen el código actual. El formato 3MF, la interfaz de plano y el guardado atómico conservan su contrato.

## Automatización de producción

`--inspect ruta.stl|ruta.3mf` y `--inspect-stdin` devuelven JSON con formato, unidades, dimensiones, triángulos, advertencias y volumen firmado. El volumen firmado por sí solo no demuestra cierre ni validez de un modelo importado.

`--cut-stdin nx ny nz d positive|negative` recibe STL/3MF por tubería y devuelve un STL binario por stdout. Repetir para obtener el otro lado. Salida 0: éxito; salida 1: error en stderr. Descartar stdout si el proceso falla. El CLI no guarda archivos ni aplica el guardado atómico de la interfaz.

`--edit-stdin` permite encadenar operaciones de edición por JSON sin escribir archivos; contrato en HISTORY.md.

## Evidencia histórica del primer incremento 0.2.0

- Compilación Release x64 local correcta, Qt 6.8.3, lib3mf 2.5.0, MSVC 19.51.36252.0, SDK Windows 10.0.26100.0. Log: `build/phase02-build.log`.
- **48/48** comprobaciones inline satisfactorias, entradas generadas en memoria: regresión STL (incluidos encabezados `solid` y `PK`), seis unidades 3MF, transformaciones y componentes, reflexión, ZIP stored/deflate, metadatos de fabricante, índices/XML/extensiones/ciclos inválidos, límites ZIP, cortes horizontal/vertical/oblicuo, normal escalada, modelo trasladado, plano pasando por un vértice, sección cóncava L, componentes disjuntos y rechazo de huecos anidados, degeneraciones y geometría no manifold. Incluye 20 planos adicionales con ambos lados, reimportación float32 y aristas cerradas/orientadas. Reporte: `build/phase02-inline-results.json`. Sin fuentes ni fixtures de pruebas propios.
- Archivo 3MF real existente: 20 124 triángulos, unidades mm; lectura por ruta correcta y SHA-256 intacto. Evidencia privada en `build/phase02-real-import.json`; no se publica ni copia la fuente.
- Interfaz visible OpenGL 3.3/NVIDIA RTX 3060 Ti: cubo 3MF de 10 mm, plano amarillo, corte con volumen 1000 → 500 + 500 mm³ y selección visible de ambos lados, 20 triángulos por pieza. Intento posterior con plano exterior muestra error y mantiene la pieza previa. Árboles de accesibilidad en `build/phase02-gui-*.txt`; capturas observadas durante la ejecución.
- Exportación asistida desde el diálogo nativo: el usuario guardó `prueba.stl` en la raíz del proyecto. La interfaz confirmó escritura/reimportación y una lectura CLI independiente comprobó 20 triángulos, dimensiones 10×10×5 y volumen firmado 500. SHA-256 `8c79e037add42bf0f227b4e3033d6a03cc1f194f9fcc34332326591b5f3d3cdf`. Se conserva como salida del usuario, sin añadirlo a Git. Evidencia: `build/phase02-gui-export.txt`.
- Mallas subdivididas generadas solo en memoria: corte completo de 756 012 triángulos y de 1 929 612 triángulos; volumen original ≈1 y dos partes de 0.5. Cada lado de la mayor tiene 971 220 triángulos. No se midieron tiempos completos de corte ni picos de memoria mediante muestreo; no se presentan como benchmark.
- Cancelación visible de corte sobre 1 929 612 triángulos mediante botón y, tras corregir el atajo, Ctrl+.; ambos conservan el modelo. Se corrigió que cambiar el texto de QAbstractButton eliminaba su atajo, utilizando QShortcut asociado a la ventana. En el paquete final se observó carga/renderizado, cancelación por Ctrl+. y cierre normal de la ventana. Árboles y logs en `build/phase02-gui-cancel-large.txt`, `build/phase02-gui-shortcut.txt` y `build/phase02-shortcut-gui.log`. No se midió latencia máxima ni se probó cancelar en cada etapa interna del corte/SDK.
- ZIP final provisional `STLCutStudioAI-0.2.0-portable-win-x64-20261108T161022669Z.zip`, SHA-256 `be91cf74216abb169623511f93eea2992424f6e5d58bb129911327814462a2af`. Inventario de 72 archivos más manifiesto, DLL y licencia del SDK incluidos. Diez comprobaciones del workflow ejecutadas localmente contra la copia extraída a ruta con espacios y ñ, con PATH limitado a Windows/System32, satisfactorias. Reporte: `build/phase02-package-verification.json`. El primer intento de empaquetado detectó la carpeta licenses ya instalada por CMake; se corrigió su creación idempotente sin borrar salidas anteriores.
- Workflow preparado para repetir esas comprobaciones en GitHub. No se declara una nueva ejecución remota ni se hace push en este incremento.

## Trabajo restante de fase 0.2

0.2.2 validada localmente con 22 casos de historial, 63 de regresión y 21 del paquete; CI remota del incremento satisfactoria en 34b0cb7 (ejecución 6). CI remota de 0.2.1 completada correctamente en el commit b1ede31; evidencia en CI.md. Continúa ampliar la validación con archivos reales y mediciones de rendimiento. No hay reparación universal, certificación completa de ausencia de autointersecciones ni guardado persistente de proyectos. Los huecos anidados y casos coplanares del motor anterior ya se abordaron en 0.2.1: no deben volver a presentarse como pendientes generales. Si se exige cancelación con plazo estricto durante el motor, deberá evaluarse un proceso auxiliar. El instalador final y VM Windows limpia permanecen en fase 1.0.

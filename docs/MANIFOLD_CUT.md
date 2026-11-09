# Corte con Manifold 3.5.4 — incremento 0.2.1

Fecha: 2026-11-08. Sustitución del primer motor de corte autorizada por el usuario, con revisión de licencia, integración, cancelación y validación de resultados. La importación STL/3MF y la exportación STL se conservan.

## Qué cambia

El corte ya no usa el ensamblado de contornos y triangulación por orejas del primer incremento. Adapta la malla a **Manifold 3.5.4**, llama `SplitByPlane` y convierte ambos lados a la representación interna. Se admiten cortes por cavidades y contornos anidados, y coincidencias con aristas/caras que antes se rechazaban de forma general. Se retiró el límite de **4096 vértices de contorno**. Esto no garantiza que cualquier malla defectuosa pueda cortarse.

Un plano que solo toca la superficie exterior no genera dos piezas de volumen positivo: se informa ese hecho y se conserva el modelo, sin desplazar artificialmente el plano. No equivale a un error por geometría coplanar. La aceptación incluye una pieza L cuyo plano atraviesa el volumen y coincide con una cara cóncava, además de un plano diagonal que contiene aristas completas.

## Dependencia y configuración reproducible

- Origen: [Manifold v3.5.4](https://github.com/elalish/manifold/tree/v3.5.4), commit `ce50d78021d64507f89e8c9fc2c2e51018117857`.
- Archivo: `https://codeload.github.com/elalish/manifold/zip/refs/tags/v3.5.4`, 31 074 873 bytes. SHA-256 calculado tras descargar la fuente oficial: `5efa97c2cd9673294be10fd6725dfc5bcc43b7fd5dd4a115a6d1b48cad1788bb`. No se presenta como firma o checksum publicado por upstream.
- Instalación explícita: `./scripts/bootstrap-manifold.ps1`. Verifica el ZIP y contrasta cada archivo de producción instalado con su entrada del ZIP. Extrae solo fuentes y metadatos necesarios; no extrae pruebas, fixtures ni ejemplos upstream en instalaciones nuevas.
- Directorio ignorado por Git: `build/deps/manifold-3.5.4/manifold-3.5.4`. No modifica herramientas ni variables persistentes del sistema.
- Compilación estática con el mismo MSVC y runtime dinámico que la aplicación. No hay DLL Manifold adicional ni dependencia Python. Opciones desactivadas: pruebas/bindings C/Python/JS, backend paralelo TBB, CrossSection/Clipper2 y descargas implícitas de CMake. Solo se usa el motor 3D y código incluido en la fuente fijada.
- Licencia Apache-2.0; se distribuyen `LICENSE` y `AUTHORS` originales bajo `licenses/manifold-3.5.4`. La fuente incluye una adaptación de linalg originalmente de Sterling Orsten, cuyo encabezado indica procedencia de dominio público y licencia Apache-2.0 para esta adaptación. Véase THIRD_PARTY_NOTICES.md. La licencia final del producto sigue pendiente; no se incorpora CGAL.

## Contrato numérico y topológico

1. Valida parámetros finitos y normal no nula. Se normalizan normal y desplazamiento conjuntamente para conservar la ecuación `n·p=d`.
2. Comprueba cierre, orientación y ausencia de triángulos degenerados a la tolerancia elegida. Recupera índices compartidos usando puntos canónicos por proximidad; el archivo y la malla original permanecen intactos. Solo la copia del adaptador usa esas posiciones, con desviación dentro de la tolerancia.
3. Centra y escala las coordenadas por la extensión máxima del modelo antes del motor. Usa MeshGL64: posiciones double e índices de 64 bits. La malla devuelta por el motor debe tener estado válido.
4. Comprueba que la adaptación conserva el volumen original; después ejecuta el corte. En cada lado elimina las fronteras de procedencia con `AsOriginal` y consolida geometría coplanar mediante `Simplify` con la tolerancia normalizada. Esto evita que restos de espesor nulo se conviertan en aristas con cuatro caras cuando STL pierde los índices originales.
5. Devuelve coordenadas al sistema del modelo, comprueba cierre/orientación de las dos piezas y conservación del volumen. Solo después se prepara y publica el nuevo resultado en la interfaz.

La tolerancia espacial sigue siendo `max(extensión × 1e-9, coordenada absoluta máxima × 2e-15)`; se rechaza si supera `extensión × 1e-5`. También se rechazan escalas cuyo cubo desborda o se anula en double. El volumen firmado original debe ser positivo. La diferencia de volumen en adaptación y corte no puede superar `max(volumen original × 1e-7, extensión³ × 1e-12)`.

La exportación de interfaz mantiene su control independiente: STL binario float32, reimportación de bytes, posiciones con error ≤ extensión × 1e-6, cierre de piezas cortadas y volumen dentro de `max(abs(volumen) × 1e-5, extensión³ × 1e-8)` antes del guardado atómico. Un modelo trasladado lejos del origen puede perder más precisión al convertir a float32; la aceptación del STL exportado usa esa tolerancia de exportación y no la del cálculo double. No se promete conservación exacta de topología para cualquier conversión float32.

## Límites de recursos y cancelación

- Se mantienen **2 millones de triángulos de entrada y por pieza**; también se limita a **2 millones de vértices indexados** de entrada y por pieza.
- Presupuesto de **512 MiB para los buffers geométricos contabilizados por el adaptador**: fuente/indexación y buffers de salida previstos a partir de conteos. No mide ni limita toda la RAM del proceso: mapas de topología, estructuras temporales de Manifold, visor y otras asignaciones son adicionales. Puede rechazar una salida aunque cada lado esté por debajo del límite individual.
- No existe un límite separado para vértices de contorno. El caso de aceptación con 8800 intersecciones estimadas de contorno valida la retirada del tope anterior; no constituye una garantía de tamaño ilimitado.
- Continúa en QThread y usa stop_token. Un stop_callback comunica cancelación al ExecutionContext durante ingestión; se comprueba el token durante indexación y conversión y antes/después de las operaciones de motor. **SplitByPlane es inmediato en 3.5.4 y no consulta ese contexto durante su operación booleana interna**; una cancelación que llegue dentro se atiende al regresar. También puede demorarse en la consolidación interna. No se promete una latencia máxima ni se termina un hilo a la fuerza.
- Fallo/cancelación descarta el trabajo y conserva el original y el resultado previo. El guardado atómico STL permanece sin cambios. Si se exige cancelación con plazo estricto, deberá evaluarse un proceso auxiliar, sin presentarlo como implementado.

## Validación local realizada

- Release x64 compilado con Qt 6.8.3, lib3mf 2.5.0, Manifold 3.5.4 y MSVC 19.51.36252.0. Registro: `build/manifold-build.log`. Se observó una advertencia C4244 en el header externo math.h en la asignación unsigned `lx=xx` del cálculo de acos: el código separa las palabras de 32 bits de un double; esa conversión conserva la palabra baja. No corresponde a índices ni al conteo de triángulos. No se modificó código upstream ni se afirma una compilación sin advertencias.
- **63/63 casos inline satisfactorios**, sin fuentes ni archivos de fixtures propios. Regresión STL/3MF, seis unidades y transformaciones, reflexión, entradas y ZIP inválidos; cubos, sección L, componentes separados, tubos cuadrados y circulares, varias cavidades, cavidad encerrada, cortes horizontales/oblicuos y coincidencias con aristas/cara cóncava. Comprueba volumen, reimportación STL, aristas con parejas orientadas y ausencia de tapas que rellenen centros de cavidades. Incluye tubo de 2200 segmentos y 8800 intersecciones de contorno. Evidencia: `build/manifold-inline-results.json`.
- Medición CLI en cubo subdividido de **1 929 612 triángulos**, generado en memoria: **13.3604 s**, pico RSS **1350.70 MiB**, pico commit **1363.88 MiB**, 445 muestras a aproximadamente 20 ms mediante GetProcessMemoryInfo. Incluye transferencia de tubería, corte completo y serialización STL; no incluye generación Python de la entrada ni GUI/GPU. Resultado positivo: 12 triángulos tras consolidación de caras planas, volumen 0.5. Es un caso plano favorable para simplificación; no representa todas las mallas de ese tamaño. Evidencia: `build/manifold-performance.json`.

- Paquete provisional 0.2.1: `STLCutStudioAI-0.2.1-portable-win-x64-20261108T165148344Z.zip`, SHA-256 `a2719a748249c91e69197d6335c3f8464ce8acc7db762fd8d782f841ffa203b0`. Inventario exacto de **74 archivos más manifiesto**, con LICENSE/AUTHORS de Manifold; no se instalan headers ni bibliotecas de desarrollo del subproyecto. Las **15 comprobaciones del workflow** pasaron localmente contra su copia extraída a ruta con espacios y ñ, con PATH limitado a Windows/System32. Comprenden cinco nuevas aceptaciones de tubo hueco, cavidad encerrada, aristas coplanares, cara cóncava coplanar y contorno superior a 4096. Se verifican aristas orientadas, reimportación, volumen y huecos sin rellenar. Reporte: `build/manifold-package-verification.json`.
- Interfaz visible de 0.2.1: OpenGL 3.3, NVIDIA GeForce RTX 3060 Ti/controlador 610.88. Tubo cuadrado de dimensiones 4×4×2, volumen 24; ambos resultados de 4×4×1, 32 triángulos y volumen 12. Observado el hueco pasante y la superficie anular nueva del lado negativo. Plano tangente z=0 rechazado por no producir dos volúmenes, conservando la pieza anterior. Árboles de accesibilidad: `build/manifold-gui-positive.txt`, `build/manifold-gui-negative.txt` y `build/manifold-gui-tangent-retention.txt`; capturas observadas en sesión.
- Cancelación visible por Ctrl+. con malla de **1 929 612 triángulos**: mensaje de cancelación y estadísticas originales conservadas. No se instrumentó la etapa interna exacta ni una latencia máxima de respuesta; no se afirma interrupción inmediata dentro de SplitByPlane. Evidencia: `build/manifold-gui-cancel.txt`, `build/manifold-cancel-gui.log`. Las dos ventanas de validación se cerraron mediante Alt+F4 y dejaron de aparecer en el inventario de ventanas; no se terminaron a la fuerza.

El workflow incorpora la adquisición verificada de Manifold y esas cinco aceptaciones adicionales. Posteriormente el usuario autorizó publicar: código subido a main como b1ede31a1a54700b2bd2495f5119ca600895a796 y CI remota completada success, con quince casos y artefacto windows-x64-5; enlaces, entorno y SHA-256 remoto en CI.md. El archivo de exportación previo `prueba.stl` sigue intacto y sin añadir a Git. No se repitió el guardado asistido desde diálogo nativo en 0.2.1; la serialización y reimportación sí se verificaron por CLI.

## Límites que siguen vigentes

No hay reparación universal, certificación completa de ausencia de autointersecciones, unión booleana automática de instancias 3MF solapadas, guardado persistente de proyectos ni exportación 3MF. Se conservan los avisos de materiales/colores y unidades de STL. El instalador final `.exe` y su validación en VM Windows limpia continúan en fase 1.0.

Desde 0.2.2 hay cortes encadenados e historial de sesión sin cambiar este contrato geométrico; véase [HISTORY.md](HISTORY.md).

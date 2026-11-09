# STL Cut Studio AI — Documento maestro para Codex

**Versión del documento:** 1.5
**Estado:** versión local 0.3.2 en validación: lectura de pintura y miniz 3.1.0 autorizada, compilación/paquete completados. Hubo evidencia preliminar de pintura sobre un modelo real, pero Windows bloqueó la ejecución final (4551); batería y regresiones finales pendientes. Detalles en [PAINT_IMPORT.md](PAINT_IMPORT.md). Insertos y partición volumétrica siguen pendientes. Sin revisión visual; publicación autorizada para validar en CI, aún pendiente de resultado. Última CI remota verificada 0.2.2.
**Plataforma:** Windows 10/11 x64  
**IDE:** Visual Studio Code + Codex  
**Objetivo:** aplicación local instalable para importar, reparar, cortar y exportar mallas 3D; posteriormente generar modelos a partir de imágenes y texto.

## 1. Instrucciones para Codex

Actúa como arquitecto de software y desarrollador senior de C++20, Qt 6, geometría computacional, Python y empaquetado Windows. Lee este documento antes de modificar el repositorio. Prioriza código compilable, pruebas automatizadas, dependencias fijadas y documentación reproducible. No declares que una función está implementada si no se ha compilado y probado. Entrega cambios pequeños, verificables y documentados. Nunca elimines archivos del usuario sin confirmación. No ejecutes modelos IA descargados de orígenes desconocidos. Evita incorporar dependencias sin revisar su licencia.

**Primera tarea:** crear el esqueleto del repositorio, una ventana Qt 6 funcional, importación STL ASCII/binaria, visor 3D, pruebas mínimas, scripts de configuración y una compilación Release reproducible. No intentar implementar simultáneamente toda la IA.

## 2. Visión del producto

STL Cut Studio AI permite:

1. Importar STL y 3MF (incorporado en fase 0.2); OBJ en una etapa posterior.
2. Visualizar, medir, rotar y transformar modelos 3D.
3. Cortar por planos horizontal, vertical, oblicuo o arbitrario; obtener componentes separados y cerrar superficies cuando la geometría lo permita.
4. Generar conectores y cavidades con tolerancias configurables. Ampliación aprobada: separar 3MF por color/material y conectar las regiones vecinas; diseño en [PHASE_03.md](PHASE_03.md), todavía sin implementación.
5. Detectar defectos y reparar mallas con previsualización y operaciones reversibles.
6. Generar mallas desde una o varias imágenes mediante modelos de IA compatibles.
7. Generar geometría paramétrica desde instrucciones mediante OpenSCAD/Blender y un LLM opcional.
8. Exportar STL, OBJ y 3MF para utilizar en laminadores externos.
9. Guardar proyectos, mantener historial y recibir actualizaciones firmadas/verificadas.

**Fuera del alcance inicial:** generación de G-code, compatibilidad garantizada con cualquier STL defectuoso, IA generativa integrada en el instalador base, certificación automática de ausencia de copyright.

## 3. Arquitectura y tecnologías

| Capa | Tecnología | Responsabilidad |
|---|---|---|
| Escritorio | C++20, Qt 6 Widgets o QML (elegir una estrategia documentada), CMake | Ventanas, menús, interacción y proyectos |
| Visor | QOpenGLWidget + Qt OpenGL/OpenGLWidgets 6.8.3, alternativa aprobada para fase 0.1 | Renderizado de mallas; plano de corte futuro |
| Geometría | Manifold 3.5.4, aprobado para corte 0.2.1; CGAL/libigl solo alternativas futuras | Corte de sólidos; ampliaciones de reparación sujetas a otro incremento |
| IA | Python 3.11, servicio local FastAPI opcional | Adaptadores de modelos imagen→3D y texto→3D |
| Paramétrico | OpenSCAD y/o Blender como herramientas externas opcionales | Modelado por instrucciones |
| Configuración | JSON, SQLite si se necesita índice de proyectos | Preferencias, proyectos y metadatos |
| Construcción | MSVC x64, CMake Presets, Ninja, vcpkg manifest | Reproducibilidad |
| Instalación | windeployqt, Inno Setup | Paquete Windows |
| Actualizaciones | WinSparkle o solución equivalente | Comprobación HTTPS y actualización controlada |
| CI | GitHub Actions Windows | Compilación, pruebas, artefactos y releases |

**Licencias:** verificar el régimen de Qt, CGAL, VTK, libigl y pesos de modelos IA antes de distribuir, en especial si será comercial. Registrar cada dependencia y su licencia en `THIRD_PARTY_NOTICES.md`.

## 4. Diseño modular

```text
STL-Cut-Studio-AI/
├── AGENTS.md
├── README.md
├── CMakeLists.txt
├── CMakePresets.json
├── vcpkg.json
├── vcpkg-configuration.json
├── .gitignore
├── .gitattributes
├── .editorconfig
├── .env.example
├── docs/
│   ├── MASTER.md
│   ├── ARCHITECTURE.md
│   ├── REQUIREMENTS.md
│   ├── WINDOWS_SETUP.md
│   ├── BUILD_AND_RELEASE.md
│   ├── TEST_PLAN.md
│   ├── LICENSE_COMPLIANCE.md
│   └── CHANGELOG.md
├── apps/desktop/
│   ├── main.cpp
│   ├── ui/
│   └── resources/
├── core/
│   ├── mesh/
│   ├── cutting/
│   ├── connectors/
│   ├── repair/
│   ├── validation/
│   ├── io/
│   └── projects/
├── services/ai/
│   ├── pyproject.toml
│   ├── requirements.in
│   ├── requirements-win-py311.lock.txt
│   ├── src/
│   │   ├── api/
│   │   ├── adapters/
│   │   ├── image_to_3d/
│   │   └── text_to_3d/
│   └── tests/
├── tests/
│   ├── fixtures/
│   ├── unit/
│   └── integration/
├── scripts/
│   ├── bootstrap-windows.ps1
│   ├── build-windows.ps1
│   ├── test-windows.ps1
│   ├── package-windows.ps1
│   └── verify-package.ps1
├── packaging/windows/
│   └── installer.iss
└── .github/workflows/
    └── windows-ci.yml
```

El código real deberá documentar interfaces y contratos de cada módulo. Evitar acoplar el motor geométrico con QML o FastAPI.

## 5. Requisitos funcionales y criterios de aceptación

### RF-01: importación
- Abrir STL ASCII y binario; desde fase 0.2 también 3MF, con geometría convertida a mm y límites de PHASE_02.md.
- Rechazar archivos truncados y registrar errores comprensibles.
- Mostrar dimensiones, cantidad de triángulos y caja envolvente.
- **Prueba:** cubo de referencia importado sin pérdida de triángulos.

### RF-02: visor
- Orbitar, zoom, paneo, encuadrar objeto, mostrar ejes y cuadrícula.
- No bloquear la UI durante tareas extensas.
- **Prueba:** modelo de prueba visible y controles funcionales.

### RF-03: cortes
- Definir plano por punto y normal; mostrar previsualización.
- Separar geometría positiva y negativa; cerrar superficies si procede.
- Preservar orientación y escala; generar archivos individuales.
- **Prueba:** cubo cerrado cortado por el centro produce dos volúmenes válidos; la suma de volúmenes coincide con el original dentro de tolerancia documentada.
- **Nota:** modelos abiertos, autointersectados o no manifold requieren tratamiento y errores explícitos; no prometer resultados cerrados universales.

### RF-04: conectores
- Pines, cavidades y holguras en mm; evitar invasión de paredes insuficientes.
- **Prueba:** unión exportada mantiene tolerancia nominal configurada.

### RF-05: reparación
- Detectar bordes abiertos, normales inconsistentes, degeneraciones y autointersecciones.
- Mostrar antes/después y permitir deshacer.

### RF-06: imagen a 3D
- Admitir PNG/JPG/WebP; recomendar 512×512 mínimo práctico y 1024×1024 objetivo, sin imponer umbrales universales.
- El motor podrá redimensionar internamente según sus requisitos.
- Modo de imagen única; modo multivista solo con backend que lo soporte.
- Mostrar estado, progreso, errores, consumo aproximado de recursos y advertencias sobre superficies inferidas.
- Validar y reparar la malla antes de exportarla para impresión.
- **Prueba:** adaptador simulado funciona sin GPU; pruebas reales condicionadas al motor instalado.

### RF-07: texto a 3D
- Generar piezas paramétricas por instrucciones con dimensiones verificables.
- No ejecutar scripts arbitrarios de IA sin aislamiento y validación.

### RF-08: proyectos
- Implementado en 0.2.2: historial de cortes en memoria, selección/plano y procedencia; límites y validación en HISTORY.md. Guardar/reabrir proyectos en disco permanece pendiente.
- Guardar archivos fuente, transformaciones, cortes, parámetros y versión de esquema.
- No sobrescribir fuentes originales por defecto.

### RF-09: exportación
- STL desde MVP; OBJ y 3MF posteriormente.
- Registrar unidades, orientación, nombre de pieza y advertencias.
- Reimportar el resultado como prueba de integridad.

### RF-10: distribución
- Entrega final obligatoria: instalador `STLCutStudioAI-Setup-<version>-win-x64.exe` con las dependencias necesarias para Windows x64.
- Tras instalar, abrir `STLCutStudioAI.exe` sin instalar Qt, Python, compiladores ni gestores de desarrollo manualmente.
- Desinstalador, accesos directos, versión visible y registro de errores.
- IA avanzada como complemento opcional separado.
- Validar instalación y ejecución final en una VM Windows limpia. No se requiere un segundo PC físico.

## 6. Dependencias: no basta con requirements.txt

Hay **tres capas de dependencias**:

1. **C++/Qt/VTK/CGAL:** `vcpkg.json` fija dependencias directas; `vcpkg-configuration.json` fija el registro/baseline. Mantener también una versión fija de Qt (instalación gestionada por script o gestor compatible) y MSVC/CMake. Documentar triplete `x64-windows` y decisiones de enlace.
2. **Python IA:** `pyproject.toml` y `requirements.in` describen dependencias; `requirements-win-py311.lock.txt` contiene versiones exactas y, cuando sea posible, hashes, generados para Windows x64/Python 3.11. No inventar versiones ni hashes: generarlos y verificarlos al integrar cada motor. `requirements.txt` puede mantenerse como entrada convencional, pero no reemplaza el lock reproducible.
3. **Runtime distribuido:** Qt DLL/plugins, runtimes de MSVC, bibliotecas C++ y, solo si se incluye IA, Python embebido y librerías compatibles. El usuario final **no debe ejecutar pip install ni instalar vcpkg** para abrir la app.

Los pesos de modelos de IA son paquetes de datos separados: versionados, con checksum SHA-256, tamaño, licencia y origen; descargables de forma explícita. Nunca asumir que `pip install` descarga y configura automáticamente un motor completo.

## 7. Un equipo de desarrollo, CI y entrega ejecutable

- Se utiliza un solo equipo Windows de desarrollo y GitHub Actions para compilación remota. El segundo PC físico deja de ser requisito y no bloquea ninguna fase.
- En desarrollo usar el repositorio, scripts y preset `windows-x64-release`; documentar diferencias reales entre toolchain local y CI.
- Nunca subir `build/`, `.venv/`, cachés, pesos de IA ni credenciales a Git.
- Fase 0.1: ZIP portátil provisional con sus DLL/plugins/runtime, comprobado localmente con PATH mínimo y mediante las comprobaciones aplicables en CI. No exige instalador ni VM limpia como condición de cierre de esta fase.
- Fase 1.0: entregar un instalador `.exe` que despliegue la aplicación y sus dependencias; validar instalación, apertura y desinstalación en una VM Windows limpia, sin Qt, Python ni Visual Studio preinstalados.
- Conservar proyectos y preferencias en `%LOCALAPPDATA%/STLCutStudioAI/` o `%APPDATA%/STLCutStudioAI/`, separados de `Program Files`.
- Mantener rutas relativas y evitar referencias a carpetas específicas del desarrollador.

Flujo diario: actualizar el repositorio → trabajar → compilar y comprobar → documentar → commit/push cuando estén autorizados. Los artefactos se producen mediante scripts y CI, no copiando carpetas de desarrollo.

## 8. Compilación y empaquetado Windows

**Desarrollo:** CMake preset + MSVC x64 + Ninja; Qt y librerías enlazadas según configuración documentada.

**Release:**
1. CI obtiene commit limpio y versiones fijadas.
2. Compila `Release` x64.
3. Ejecuta CTest y pruebas de integración.
4. Instala binarios en directorio de staging.
5. Ejecuta `windeployqt` y agrega runtimes/bibliotecas faltantes según licencias.
6. Comprueba plugins de plataforma (`platforms/qwindows.dll`), TLS y rutas relativas.
7. Crea instalador con Inno Setup.
8. Firma instalador/binarios cuando exista certificado; calcula SHA-256.
9. Prueba instalación, apertura, carga de STL, corte, exportación y desinstalación en Windows limpio.
10. Publica artefactos versionados y notas de versión.

**Artefactos esperados:** `STLCutStudioAI-Setup-<version>-win-x64.exe`, `SHA256SUMS.txt`, `CHANGELOG.md`, manifiesto de dependencias y reporte de pruebas.

**Compatibilidad:** Windows 10/11 x64, versión mínima del SO y arquitectura fijadas en CI; no garantizar compatibilidad con todas las GPU/controladores. Si un equipo no tiene GPU compatible, el corte tradicional debe seguir funcionando.

## 9. Actualizaciones

- Consultar por HTTPS un feed de versiones; comparar versión instalada con disponible.
- Mostrar cambios y solicitar consentimiento antes de actualizar.
- Descargar instalador firmado, comprobar integridad/autenticidad y cerrar la aplicación de manera segura antes de instalar.
- No sobrescribir proyectos ni configuraciones del usuario.
- Separar noticias del producto de paquetes ejecutables; las noticias no ejecutan código.
- Probar actualización desde la versión anterior y contemplar recuperación ante fallo.

## 10. Seguridad, privacidad y copyright

- Procesamiento local por defecto; sin subir STL/imágenes sin consentimiento explícito.
- Validar tamaños de archivo y formatos; tratar STL e imágenes como datos no confiables.
- Aislar procesos de IA; límites de memoria/tiempo y registro de fallos.
- No ejecutar scripts generados por LLM sin validación, sandbox o restricciones explícitas.
- Conservar metadatos de procedencia de imágenes, prompts, modelos y licencias.
- La conversión imagen→3D no elimina derechos de autor ni garantiza originalidad.
- Verificar licencias de código, dependencias y pesos IA antes de distribución comercial.

## 11. Estrategia de pruebas

- **Unitarias:** parser STL, matrices, planos, geometría, serialización.
- **Integración:** importación → corte → exportación → reimportación.
- **Robustez:** STL malformados, enormes, sin triángulos, abiertos y autointersectados.
- **Rendimiento:** mallas de 10k, 100k y 1M triángulos; medir RAM, tiempo y capacidad de cancelar.
- **Instalación:** Windows limpio, usuario sin privilegios elevados cuando sea viable, rutas Unicode y espacios.
- **IA:** mocks en CI; pruebas de inferencia en hardware compatible y matriz de GPU/VRAM.
- **Actualización:** instalación limpia, actualización desde N-1, reversión o recuperación.

No publicar versión 1.0 sin validar el instalador `.exe` y la aplicación instalada en una VM Windows limpia. No se exige validación en dos computadores físicos.

## 12. Roadmap y entregables

| Fase | Resultado | Condición de salida |
|---|---|---|
| 0.1 | Ventana Qt, STL, visor, CI y ZIP provisional | Compilación local y CI satisfactorias, funciones verificadas y paquete con runtime comprobado; sin segundo PC ni VM obligatoria en esta fase |
| 0.2 | Importación 3MF, corte por plano, exportación STL e historial de sesión | Unidades/transformaciones correctas, límites explícitos, volumen y reimportación comprobados |
| 0.3 | Conectores, separación por color, validación y reparación | Fixtures y tolerancias verificadas |
| 0.4 | Imagen→3D con adaptador IA | Generación + validación de malla |
| 0.5 | Texto→3D y asistente | Operaciones verificables y seguras |
| 1.0 | Instalador `.exe`, actualizador y documentación | Instalación, ejecución y desinstalación verificadas en VM Windows limpia, sin preparación manual de dependencias |

Estimación inicial orientativa: varios meses de desarrollo iterativo; no asumir fechas cerradas antes de un prototipo del motor geométrico y pruebas de hardware.

## 13. Primer sprint para Codex (ejecutar en orden)

1. Crear estructura de carpetas y archivos de documentación.
2. Implementar `CMakeLists.txt`, presets y manifiesto vcpkg con versiones reales verificadas.
3. Configurar app Qt y ventana principal con menú Archivo > Abrir STL.
4. Implementar parser/importador STL con pruebas de ASCII/binario y errores.
5. Integrar visor 3D y controles de cámara.
6. Añadir fixture STL simple, tests y ejecución mediante CTest.
7. Crear scripts PowerShell idempotentes de bootstrap, build y test.
8. Configurar GitHub Actions Windows con versiones fijadas y artefacto de prueba.
9. Preparar ZIP portátil inicial y comprobar que carga sus dependencias con PATH mínimo; conservar evidencia local y de CI.
10. Documentar comandos reales ejecutados, errores, resultados y pendientes.

### Definición de terminado por tarea

- Código compilado en Release.
- Prueba automatizada correspondiente ejecutada.
- Ninguna dependencia nueva sin licencia y versión documentadas.
- README y CHANGELOG actualizados cuando aplique.
- Capturas o logs de pruebas para funciones visuales.
- Registrar el entorno realmente comprobado. La VM Windows limpia es obligatoria para la entrega final 1.0 y no se exige un segundo PC físico.

## 14. Prompt de inicio para Codex

```text
Lee docs/MASTER.md y AGENTS.md. Implementa únicamente el sprint 0.1 de STL Cut Studio AI en Windows x64. Antes de programar, inspecciona el repositorio y propón un plan breve con dependencias y riesgos. Usa C++20, Qt 6, CMake, MSVC y una estrategia reproducible para bibliotecas. Crea una app de escritorio compilable que importe STL ASCII/binario y los muestre en un visor 3D. Añade pruebas, scripts PowerShell, CI y documentación. No inventes resultados de compilación ni dependencias. Si una herramienta no está instalada, indica el comando concreto para instalarla y continúa con lo que sea verificable. No implementes IA ni conectores todavía. Al terminar, entrega archivos modificados, comandos para compilar, pruebas ejecutadas y pendientes.
```

## 15. Comandos orientativos del equipo de desarrollo

```powershell
# Ejecutar desde una terminal PowerShell en el repositorio:
git pull
powershell -ExecutionPolicy Bypass -File .\scripts\bootstrap-windows.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\build-windows.ps1
powershell -ExecutionPolicy Bypass -File .\scripts\test-windows.ps1
```

**Importante:** estos scripts son entregables que Codex debe implementar. No se afirma que existan aún. Los comandos definitivos deberán aparecer en `docs/WINDOWS_SETUP.md` después de la primera compilación comprobada.

## 16. Riesgos y decisiones pendientes

- Confirmar uso personal, código abierto o comercial para seleccionar licencias de Qt/CGAL y modelos IA.
- Elegir Qt Widgets o QML según la integración y madurez del visor VTK.
- Seleccionar motor de imagen→3D compatible con VRAM de 8 GB y verificar licencia, versión y requisitos reales.
- Elegir el formato nativo de proyecto y estrategia de migraciones.
- Definir política de firma, hosting de releases y distribución de modelos IA.
- Medir rendimiento de cortes complejos antes de prometer tiempos máximos.

**Regla de diseño:** la aplicación de corte debe funcionar aun cuando el motor de IA no esté instalado, no haya internet o falle la GPU.

## 17. Aclaraciones de alcance y estado documental (2026-11-05)

La reorganización documental no representa implementación. Los documentos complementarios son:

- [Requisitos de fase 0.1](REQUIREMENTS.md).
- [Arquitectura y decisiones pendientes](ARCHITECTURE.md).
- [Plan de pruebas](TEST_PLAN.md).
- [Registro de cambios](CHANGELOG.md).

El alcance inicial es ventana, importación STL, visor y pipeline verificable. La fase 0.1 prepara un ZIP Release provisional con sus dependencias, comprobado localmente con PATH mínimo y mediante verificaciones aplicables en CI. El instalador `.exe` completo, desinstalación y actualización pertenecen a la fase 1.0, cuya validación final será en VM Windows limpia. En fase 0.1 no se exige probar corte ni exportación, que comienzan en fase 0.2.

La estructura del apartado 4 es objetivo de evolución, no inventario de archivos existentes. Los comandos del apartado 15 siguen siendo orientativos y no ejecutables hasta implementar y verificar sus scripts. Python, FastAPI, CGAL, libigl, modelos IA y sus locks no deben añadirse solo por aparecer en la arquitectura futura; se incorporarán en la fase que los necesite, previa revisión y aprobación.

Este apartado conserva el origen documental de la planificación. El estado actual de herramientas, dependencias, límites, rendimiento, CI y paquete está en los documentos complementarios; la licencia de distribución y la validación final de la VM siguen pendientes.

## 18. Decisión vigente de entrega final (2026-11-07)

Por instrucción del usuario, se elimina el segundo PC físico como requisito. La entrega del programa completo será un instalador `.exe` para Windows x64, con dependencias necesarias y apertura de la aplicación sin preparación manual de Qt, Python ni herramientas de desarrollo. Se verificará instalación, ejecución de las funciones entregadas y desinstalación en VM Windows limpia antes de cerrar fase 1.0. El ZIP portátil actual sigue siendo provisional.

Esta actualización modifica documentación y criterios de aceptación; no implementa el instalador, no repite validaciones y no altera los ZIP/manifiestos ya generados. Las referencias históricas del CHANGELOG a un segundo PC describen el alcance anterior y no son requisitos vigentes.

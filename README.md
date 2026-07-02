# Programación III: Proyecto Final

# Netflix: Buscador inteligente de películas

> Motor de búsqueda en **C++20** para consultar películas desde un dataset CSV de Wikipedia. El sistema permite buscar por palabra, frase, sub-palabra y tags; muestra resultados paginados, permite dar **Like**, guardar en **Ver más tarde** y generar recomendaciones simples a partir del historial del usuario.

---

## Integrantes

| Integrante | Rol principal dentro del proyecto |
|---|---|
| Maydelith Zuñiga | Limpieza y preprocesamiento del CSV |
| Joaquin Llallire | Flujo de vistas, menú e interacción por consola |
| Adrián Gamboa | Motor de búsqueda, ranking y filtrado por tags |
| Carlos Condor | Árbol de sufijos, documentación y análisis de complejidad |
| Rodrigo Huertos | Pruebas, integración, rendimiento y presentación |

> Nota: la participación final debe verificarse con el historial de commits de GitHub y con la exposición del equipo.

---

## Índice

- [Descripción general](#descripción-general)
- [Funcionalidades principales](#funcionalidades-principales)
- [Arquitectura del proyecto](#arquitectura-del-proyecto)
- [Compilación y ejecución](#compilación-y-ejecución)
- [Preprocesamiento de datos](#preprocesamiento-de-datos)
- [Flujo completo del sistema](#flujo-completo-del-sistema)
- [Estructura de datos: Árbol de Sufijos Comprimido](#estructura-de-datos--árbol-de-sufijos-comprimido-patricia-trie)
- [Algoritmo de búsqueda](#algoritmo-de-búsqueda)
- [Algoritmo de inserción](#algoritmo-de-inserción--árbol-de-sufijos)
- [Patrones de diseño utilizados](#patrones-de-diseño-utilizados)
- [Programación paralela](#programación-paralela)
- [Eficiencia y resultados de rendimiento](#eficiencia-y-resultados-de-rendimiento)
- [Control de errores y excepciones](#control-de-errores-y-excepciones)
- [Uso de la STL y paradigmas de C++](#uso-de-la-stl-y-paradigmas-de-c)
- [Checklist de rúbrica](#checklist-de-rúbrica)
- [Fuentes bibliográficas](#fuentes-bibliográficas-formato-apa)

---

## Descripción general

El proyecto implementa una mini plataforma tipo **Netflix** que administra la búsqueda y visualización de películas usando un archivo CSV con **34,886 registros**. El programa no se limita a buscar coincidencias exactas: también permite encontrar películas por subcadenas, por ejemplo buscar `bar` y obtener coincidencias relacionadas con palabras que contienen ese patrón.

El sistema se apoya en tres ideas principales:

| Componente | Propósito |
|---|---|
| `limpiezadatos.h` | Limpia el CSV original y genera `wiki_movie_plots_deduped_final.csv` |
| `MotorBusqueda.h` | Carga películas, tokeniza texto, busca por palabra/tag, ordena resultados y recomienda películas |
| `SuffixTree.h` | Implementa un árbol de sufijos comprimido genérico para acelerar la recuperación de candidatos |
| `Vistas.h` | Controla la interfaz por consola, paginación, likes y listas del usuario |
| `Consoleutils.h` | Centraliza lectura segura de opciones y texto |
| `main.cpp` | Orquesta el flujo principal del programa |

---

## Funcionalidades principales

| Funcionalidad | Estado | Evidencia en código |
|---|---:|---|
| Lectura de dataset CSV grande | ✅ | `cargarCSV()` |
| Limpieza de datos antes de cargar | ✅ | `limpiezadatos::limpiardatoscsv()` |
| Búsqueda por palabra clave | ✅ | `BuscadorPorTexto` |
| Búsqueda por frase | ✅ | `tokenizar()` + unión de candidatos |
| Búsqueda por sub-palabra | ✅ | `SuffixTree::insertar()` + `buscar()` |
| Búsqueda por tag | ✅ | `TipoTag`, `BuscadorPorTag`, `buscarPorTag()` |
| Tags soportados | ✅ | Director, actor, género y año |
| Ranking de importancia | ✅ | `RankingStrategy`, `RankingPorRelevancia`, `RankingPorAnio` |
| Resultados de 5 en 5 | ✅ | `ResultadosPaginador` |
| Ver sinopsis de película | ✅ | `mostrarDetallePelicula()` |
| Like | ✅ | `ToggleLikeCommand` |
| Ver más tarde | ✅ | `ToggleVerMasTardeCommand` |
| Recomendaciones | ✅ | `obtenerPeliculasRecomendadas()` |
| Persistencia simple de likes | ✅ | `likes.txt` |
| Programación paralela | ✅ | `std::async`, `std::future`, `thread::hardware_concurrency()` |
| 4 patrones de diseño | ✅ | Strategy, Factory Method, Iterator, Command |

---

## Arquitectura del proyecto

```txt
PROYECTO_PROGRAMACION3-main/
├── main.cpp
├── MotorBusqueda.h
├── SuffixTree.h
├── Vistas.h
├── Consoleutils.h
├── limpiezadatos.h
├── CMakeLists.txt
├── wiki_movie_plots_deduped.csv
├── wiki_movie_plots_deduped_final.csv
├── likes.txt
├── proyecto.md
└── pseudoCodigo.md
```

### Organización por responsabilidades

```txt
main.cpp
   ↓
limpiezadatos.h      → Limpia el CSV original
   ↓
MotorBusqueda.h      → Carga, tokeniza, indexa, busca, rankea y recomienda
   ↓
SuffixTree.h         → Estructura de datos principal
   ↓
Vistas.h             → Interfaz, navegación, paginación y acciones del usuario
   ↓
Consoleutils.h       → Entrada segura y validaciones de consola
```

Esta separación permite un **buen nivel de abstracción**: la vista no necesita saber cómo funciona el árbol, el `main` no contiene lógica de búsqueda compleja y las acciones del usuario se encapsulan en clases independientes.

---

## Compilación y ejecución

### Requisitos

- Compilador compatible con **C++20**.
- Dataset `wiki_movie_plots_deduped.csv` en la misma carpeta del ejecutable.
- Sistema operativo Windows, Linux o macOS con consola compatible.

### Compilar con `g++`

```bash
g++ -std=c++20 -O2 main.cpp -o NetflixBuscador
```

### Ejecutar en Linux/macOS

```bash
./NetflixBuscador
```

### Ejecutar en Windows

```bash
NetflixBuscador.exe
```

### Compilar con CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

> El programa genera o actualiza automáticamente `wiki_movie_plots_deduped_final.csv` a partir del archivo original.

---

## Preprocesamiento de Datos

El dataset utilizado es `wiki_movie_plots_deduped.csv`, un archivo CSV con información de películas extraída de Wikipedia. Antes de ser usado por el sistema, pasa por una etapa de limpieza implementada en la clase `limpiezadatos`.

### ¿Qué limpia?

El procesamiento recorre el archivo **línea por línea**, reconstruyendo registros que pueden venir partidos por saltos de línea dentro de la sinopsis. Luego procesa las filas en paralelo para no cargar trabajo textual pesado en un solo hilo.

Por cada celda aplica tres limpiezas:

- **Espacios y tabs en los extremos** — elimina espacios, tabs y saltos de línea al inicio y final de cada celda.
- **Valores desconocidos** — normaliza celdas vacías o con valor `Unknown` en cualquier variante a `unknown`.
- **Saltos de línea internos** — reemplaza `\r\n`, `\r` y `\n` dentro de sinopsis por espacios.
- **Comillas internas** — escapa comillas de la sinopsis duplicándolas para conservar un CSV válido.
- **Columnas incompletas** — rellena campos faltantes para mantener las 8 columnas esperadas.

### Resumen de transformaciones

| Valor original | Resultado |
|---|---|
| *(vacío)* o `"   "` | `unknown` |
| `Unknown` / `"Unknown"` | `unknown` |
| `texto\r\ntexto` | `texto texto` |
| Sinopsis con comillas internas | Comillas escapadas según formato CSV |
| Fila con columnas faltantes | Se completa para conservar estructura |
| Cualquier otro valor válido | Se conserva normalizado |

---

## Preprocesamiento de datos — De CSV a tokens

El preprocesamiento transforma el archivo CSV crudo en palabras listas para ser insertadas en el árbol de sufijos. Se realiza en tres etapas.

---

### Etapa 1 — Carga y estructuración

`cargarCSV()` lee el archivo `wiki_movie_plots_deduped_final.csv` y construye un `unordered_map<int, Pelicula>`. Cada objeto `Pelicula` almacena los campos principales del dataset:

```txt
Release Year,Title,Director,Cast,Genre,Plot
      ↓
Pelicula {
    id, anio, titulo, sinopsis,
    directores → ["Steven Spielberg"]
    actores    → ["Tom Hanks", "Robin Wright"]
    generos    → ["drama", "romance"]
}
```

Como la sinopsis puede ocupar varias líneas en el CSV, el cargador detecta el inicio de cada película por el patrón de año al inicio de línea:

```txt
1995,...
```

Es decir, una nueva película se reconoce cuando una línea empieza con **4 dígitos seguidos de coma**.

---

### Etapa 2 — Limpieza y tokenización

Una vez construido el objeto `Pelicula`, cada campo de texto pasa por dos funciones antes de llegar al árbol:

**`limpiar(texto)`** — normaliza el texto para que las comparaciones sean consistentes. Convierte todo a minúsculas y elimina puntuación como `.`, `,`, `(`, `)`, `"`, `'`, `-`, entre otros.

```txt
"Fight! He runs." → "fight he runs"
```

**`tokenizar(texto)`** — divide el texto limpio en palabras individuales usando espacios como separador.

```txt
"fight he runs" → ["fight", "he", "runs"]
```

---

### Etapa 3 — Inserción al árbol

Con los tokens listos, `indexarCatalogo()` los inserta en el `SuffixTree<int>` asociando cada token al `id` de su película.

```txt
pelicula.id = 42

tokenizar(titulo)   → ["kansas", "saloon"]       → arbol.insertar("kansas", 42)
tokenizar(sinopsis) → ["bartender", "beer", ...] → arbol.insertarPalabraCompleta("bartender", 42)
tokenizar(director) → ["spielberg"]              → arbol.insertar("spielberg", 42)
tokenizar(actores)  → ["tom", "hanks"]          → arbol.insertar("tom", 42)
tokenizar(generos)  → ["drama"]                  → arbol.insertar("drama", 42)
```

> El árbol nunca almacena objetos `Pelicula`, solo enteros. Cuando el usuario busca, el árbol devuelve IDs y con esos IDs se recuperan las películas desde el `unordered_map<int, Pelicula>`.

---

## Flujo completo del sistema

```txt
CSV original
   ↓
limpiezadatos::limpiardatoscsv()
   ↓
CSV final limpio
   ↓
cargarCSV()
   ↓
unordered_map<int, Pelicula>
   ↓
indexarCatalogo()
   ↓
limpiar() + tokenizar()
   ↓
SuffixTree<int>
   ↓
BuscadorFactory
   ├── BuscadorPorTexto
   └── BuscadorPorTag
   ↓
RankingStrategy
   ├── RankingPorRelevancia
   └── RankingPorAnio
   ↓
ResultadosPaginador
   ↓
Vista de resultados + detalle + Like + Ver más tarde
```

---

## Procesamiento de la consulta del usuario

Antes de realizar la búsqueda en el árbol, la consulta ingresada por el usuario pasa por un proceso de normalización para garantizar que las palabras sean comparables con los tokens almacenados en la estructura.

```txt
INICIO

    consultaUsuario ← leerTexto()
    // ejemplo: "Steven Spielberg"

    textoLimpio ← limpiar(consultaUsuario)
    // "Steven Spielberg" → "steven spielberg"

    tokens ← tokenizar(textoLimpio)
    // "steven spielberg" → ["steven", "spielberg"]

    PARA CADA token EN tokens:
        ids ← arbol.buscar(token)
        acumular ids sin duplicados

FIN
```

La búsqueda de frases se maneja como una unión de resultados. Por ejemplo:

```txt
"barco fantasma" → ["barco", "fantasma"]
resultado final  → películas que contienen "barco" y/o "fantasma"
```

---

## Estructura de Datos — Árbol de Sufijos Comprimido (Patricia Trie)

La estructura de datos escogida es un **Árbol de Sufijos Comprimido**, también conocido como variante de **Patricia Trie** aplicada a sufijos. Fue elegida porque el proyecto requiere buscar películas no solo por palabras completas, sino también por sub-palabras.

Por ejemplo, si el usuario busca:

```txt
bar
```

el programa puede encontrar coincidencias relacionadas con palabras como:

```txt
barco
barca
bartender
embarcar
```

### ¿Cómo se insertan las palabras?

Para lograrlo, el árbol no inserta únicamente la palabra completa sino **todos sus sufijos** en campos cortos como título, género, director y actor.

Para la palabra `barco` se insertan:

```txt
barco
arco
rco
co
o
```

Esto garantiza que cualquier búsqueda por subcadena encuentre un match, porque esa subcadena aparecerá como prefijo de alguno de los sufijos insertados.

---

### Decisión de diseño: sinopsis y uso de memoria

La sinopsis puede contener cientos o miles de palabras por película. Insertar todos los sufijos de todas las palabras de las sinopsis de más de 34 mil películas aumentaría demasiado el uso de memoria.

Por eso se tomó la siguiente decisión:

| Campo | Método de indexación | Motivo |
|---|---|---|
| Título | `insertar()` | Campo corto y de alta prioridad |
| Género | `insertar()` | Campo corto, útil para tags |
| Director | `insertar()` | Campo corto, útil para tags |
| Actor | `insertar()` | Campo corto, útil para tags |
| Sinopsis | `insertarPalabraCompleta()` | Control de memoria en dataset grande |

Si el árbol no encuentra candidatos para una subcadena dentro de sinopsis, el programa usa un **fallback paralelo** con `buscarSustringEnSinopsisParalelo()`.

---

### Compresión de caminos

A diferencia de un Trie clásico que crea un nodo por letra, el Patricia Trie comprime secuencias de nodos en una sola arista con etiqueta.

```txt
Trie clásico:        Patricia Trie:

b                    "barco" → movie_id: 42
└─ a
   └─ r
      └─ c
         └─ o → movie_id: 42
```

Cuando dos palabras comparten un prefijo, el nodo se divide exactamente en el punto de divergencia:

```txt
Insertar "barco" y "barca":

"bar"
├─ "co" → movie_id: 42
└─ "ca" → movie_id: 17
```

---

### Cada nodo guarda IDs, no objetos

Los nodos almacenan únicamente **enteros** que representan el `id` de cada película. Al recuperar resultados, esos IDs se usan para acceder directamente al catálogo.

```txt
nodo "barco" → [42, 103, 987]
        ↓
catalogo[42]  → Pelicula { titulo, sinopsis, ... }
catalogo[103] → Pelicula { titulo, sinopsis, ... }
catalogo[987] → Pelicula { titulo, sinopsis, ... }
```

Esto evita duplicar objetos grandes dentro del árbol.

---

### Rendimiento esperado de búsqueda

| | Búsqueda lineal | Árbol de Sufijos |
|---|---|---|
| Recorre todo el dataset | ✅ siempre | ❌ no en el caso promedio |
| Tiempo de navegación | `O(n)` | `O(m)` |
| Depende de | número de películas | longitud del patrón |
| Soporta subcadena | ❌ no directamente | ✅ sí |
| Devuelve candidatos por ID | ❌ no | ✅ sí |

Donde:

```txt
n = número de películas
m = longitud del patrón buscado
k = cantidad de resultados encontrados
```

La búsqueda navega el árbol consumiendo el patrón carácter por carácter hasta agotarlo, luego recolecta todos los IDs del subárbol resultante.

---

## Algoritmo de Búsqueda

El algoritmo se divide en dos fases: **navegación** y **recolección**.

### Fase 1 — Navegación (`_navegar`)

Consume el patrón carácter por carácter bajando por los nodos del árbol:

```txt
buscar("bar")
        ↓
raiz → ¿tiene hijo que empiece con 'b'?
        ↓ sí
hijo con etiqueta "barco"
        ↓
¿"barco" contiene "bar" como prefijo?
        ↓ sí
patrón agotado → llegamos al nodo
```

Hay tres casos posibles al comparar el patrón con la etiqueta del nodo:

```cpp
// caso 1 — patrón se agota dentro de la etiqueta → encontrado
if (coincide >= patron.size())
    return hijo;

// caso 2 — etiqueta se agota antes que el patrón → bajar al siguiente nodo
if (coincide < patron.size() && coincide == hijo->etiqueta.size())
    return _navegar(hijo, patron.substr(coincide));

// caso 3 — divergencia → no existe
return nullptr;
```

---

### Fase 2 — Recolección (`_recolectar`)

Desde el nodo donde llegó la navegación, recorre todo el subárbol hacia abajo recolectando todos los `movie_ids`.

```txt
nodo "bar"
├─ "co" → [42, 103]
│   └─ "s" → [17]
└─ "ca" → [987]

recolectar → [42, 103, 17, 987]
```

```cpp
void _recolectar(nodo, out) {
    for (int id : nodo->movie_ids) out.push_back(id);
    for (auto& [_, hijo] : nodo->hijos) _recolectar(hijo, out);
}
```

---

### Complejidad

| Fase | Complejidad | Depende de |
|---|---:|---|
| Navegación | `O(m)` | longitud del patrón `m` |
| Recolección | `O(k)` | cantidad de resultados `k` |
| Deduplicar | `O(k log k)` | ordenar los IDs encontrados |

> El dataset completo no afecta el tiempo de navegación. Afecta principalmente la cantidad de resultados recolectados.

---

## Algoritmo de inserción — Árbol de Sufijos

**Entrada:** un token, por ejemplo `"barco"`, y un `movie_id`, por ejemplo `42`.

### Generación de sufijos

Lo primero que hace `insertar()` es generar todos los sufijos del token y llamar a `_insertar()` para cada uno.

```txt
"barco" → "barco", "arco", "rco", "co", "o"
```

Esto convierte la estructura en un árbol capaz de buscar subcadenas, no solo prefijos.

---

### Los 4 casos de `_insertar()`

**Caso 1 — No existe hijo con ese carácter**

Se crea una hoja directamente.

```txt
árbol vacío + insertar("barco", 42)
────────────────────────────────────
raíz → ["barco"] {42}
```

**Caso 2 — Coincidencia exacta con la etiqueta**

El sufijo es idéntico a la etiqueta del nodo; solo se anota el ID.

```txt
insertar("barco", 17) cuando ya existe ["barco"] {42}
─────────────────────────────────────────────────────
raíz → ["barco"] {42, 17}
```

**Caso 3 — El sufijo es más corto que la etiqueta**

El nodo existente se parte: el prefijo común sube y el resto queda como hijo.

```txt
existe ["barco"] {42} + insertar("bar", 99)
────────────────────────────────────────────
raíz → ["bar"] {99}
        └─ ["co"] {42}
```

**Caso 3b — La etiqueta está contenida en el sufijo**

Se anota el ID en el nodo actual y se baja recursivamente con el resto del sufijo.

```txt
existe ["bar"] {x} + insertar("barco", 42)
───────────────────────────────────────────
anota 42 en ["bar"], luego desciende con "co"
```

**Caso 4 — Coincidencia parcial**

Se crea un nodo intermedio con el prefijo común y dos hojas con los restos.

```txt
existe ["barco"] {42} + insertar("barca", 17)
──────────────────────────────────────────────
raíz → ["bar"] {17}
        ├─ ["co"] {42}
        └─ ["ca"] {17}
```

> El nodo `"bar"` recibe el ID porque `"barca"` pasó por él, no porque `"bar"` sea necesariamente una palabra completa en el catálogo.

---

### Detalles de implementación

`_anotar()` evita duplicados revisando con `std::find` antes de hacer `push_back` en `movie_ids`.

```cpp
if (std::find(nodo->movie_ids.begin(), nodo->movie_ids.end(), valor) == nodo->movie_ids.end()) {
    nodo->movie_ids.push_back(valor);
}
```

Una vez terminadas todas las inserciones, `buscar()` navega hasta el nodo que agota el patrón y recolecta todos los IDs del subárbol, pasándolos por `_deduplicar()` antes de devolverlos.

---

## Patrones de diseño utilizados

El proyecto utiliza **4 patrones de diseño** de forma explícita. No se agregaron únicamente para cumplir la rúbrica: cada patrón resuelve un problema concreto de organización, extensibilidad o separación de responsabilidades.

| Patrón | Archivo / clases | Problema que resuelve | Beneficio |
|---|---|---|---|
| **Strategy** | `RankingStrategy`, `RankingPorRelevancia`, `RankingPorAnio` | Cambiar el criterio de ranking sin modificar la búsqueda | Permite extender nuevos rankings |
| **Factory Method** | `BuscadorFactory`, `BuscadorPorTexto`, `BuscadorPorTag` | Crear el buscador correcto según el tipo de consulta | Evita `if/else` grandes en `main.cpp` |
| **Iterator** | `ResultadosIterator`, `ResultadosPaginador` | Recorrer resultados de 5 en 5 sin exponer el vector completo | Interfaz similar a contenedores STL |
| **Command** | `ComandoUsuario`, `ToggleLikeCommand`, `ToggleVerMasTardeCommand` | Encapsular acciones del usuario como objetos | Separa la vista de la lógica de modificación |

---

### 1. Strategy — Ranking intercambiable

El ranking de resultados se define mediante una interfaz común:

```cpp
class RankingStrategy {
public:
    virtual int calcularScore(const Pelicula& p, const string& consulta) const = 0;
    virtual ~RankingStrategy() = default;
};
```

Luego, el sistema puede usar distintas estrategias:

```txt
RankingStrategy
├── RankingPorRelevancia
└── RankingPorAnio
```

Ejemplo de uso:

```cpp
RankingPorRelevancia estrategia;
return ordenarConEstrategia(resultados, consulta, estrategia);
```

Para búsqueda por año, el programa cambia a `RankingPorAnio`, lo que permite priorizar coincidencias exactas o años cercanos.

---

### 2. Factory Method — Creación de buscadores

`BuscadorFactory` decide qué buscador crear según la opción elegida por el usuario.

```cpp
auto buscador = BuscadorFactory::crearBuscador(esBusquedaPorTag, tipoTag);
resultados = buscador->buscar(db, arbolBusqueda, consulta);
```

```txt
BuscadorFactory
├── BuscadorPorTexto
└── BuscadorPorTag
```

Esto evita que `main.cpp` tenga que conocer todos los detalles de búsqueda.

---

### 3. Iterator — Paginación de resultados

`ResultadosPaginador` expone `begin()` y `end()` para recorrer únicamente la página actual.

```cpp
for (const Pelicula& p : paginador) {
    std::cout << p.titulo << "\n";
}
```

La vista no necesita manejar índices globales del vector completo.

```txt
resultados completos
[0][1][2][3][4][5][6][7][8][9]...
       ↓
página actual
[0][1][2][3][4]
```

---

### 4. Command — Acciones del usuario

Las acciones de Like y Ver más tarde se encapsulan como comandos:

```cpp
ToggleLikeCommand comando(likes, p.id);
comando.ejecutar();
```

Esto permite que la vista no modifique directamente la lógica interna de las listas.

```txt
ComandoUsuario
├── ToggleLikeCommand
└── ToggleVerMasTardeCommand
```

---

## Programación paralela

El proyecto usa programación paralela con componentes estándar de C++:

```cpp
std::async
std::future
std::thread::hardware_concurrency()
```

### Módulos paralelizados

| Módulo | Función | Estrategia paralela |
|---|---|---|
| Limpieza de CSV | `limpiardatoscsv()` | Divide filas en chunks y procesa cada bloque con `std::async` |
| Carga de CSV | `cargarCSV()` | Divide registros completos entre hilos y combina resultados |
| Fallback de búsqueda en sinopsis | `buscarSustringEnSinopsisParalelo()` | Divide películas entre hilos cuando el árbol no encuentra candidatos |

### Esquema de paralelización

```txt
vector de filas / películas
        ↓
+---------------+---------------+---------------+---------------+
| chunk hilo 1  | chunk hilo 2  | chunk hilo 3  | chunk hilo 4  |
+---------------+---------------+---------------+---------------+
        ↓               ↓               ↓               ↓
    future 1        future 2        future 3        future 4
        ↓               ↓               ↓               ↓
        +---------------+---------------+---------------+
                        ↓
             combinación de resultados
```

Se eligió `std::async` porque permite lanzar tareas asíncronas sin administrar manualmente objetos `std::thread`, y `std::future` permite recuperar los resultados de cada tarea de forma segura.

---

## Eficiencia y resultados de rendimiento

Se consideraron criterios de eficiencia de **tiempo** y **espacio** en el diseño del proyecto.

### Decisiones de eficiencia

| Decisión | Impacto |
|---|---|
| `unordered_map<int, Pelicula>` para el catálogo | Acceso promedio `O(1)` por ID |
| El árbol guarda IDs, no objetos completos | Menor duplicación de memoria |
| Patricia Trie / árbol comprimido | Menos nodos que un Trie clásico |
| `insertar()` solo en campos cortos | Control de memoria |
| `insertarPalabraCompleta()` en sinopsis | Evita explosión de sufijos |
| `std::sort` + `std::unique` | Deduplicación eficiente de IDs |
| Paginación de 5 resultados | Evita mostrar listas enormes en consola |
| Fallback paralelo | El escaneo completo solo se usa en peor caso |

---

### Tabla comparativa de tiempos

Medición referencial local usando el dataset completo de **34,886 películas** y compilación con:

```bash
g++ -std=c++20 -O2
```

| Operación | Versión base | Versión optimizada | Mejora aproximada |
|---|---:|---:|---:|
| Limpieza de CSV | 1415 ms | 1272 ms | 10.1% |
| Búsqueda `spielberg` | 521 ms | 1 ms | 99.8% |
| Búsqueda `war` | 674 ms | 251 ms | 62.8% |
| Búsqueda `bar` | 516 ms | 159 ms | 69.2% |
| Búsqueda `love` | 528 ms | 292 ms | 44.7% |

| Concepto | Versión base | Versión optimizada |
|---|---|---|
| Limpieza de CSV | Recorrido secuencial | Procesamiento por chunks con `std::async` |
| Búsqueda | Escaneo de todas las películas | Candidatos por `SuffixTree<int>` |
| Sinopsis sin candidatos | Escaneo completo directo | Escaneo paralelo solo como fallback |

> Los tiempos pueden variar según procesador, almacenamiento, sistema operativo y carga actual del equipo. La tabla sirve como evidencia de comparación, no como valor absoluto universal.

---

## Control de errores y excepciones

El sistema incluye validaciones para evitar fallos comunes durante la ejecución.

| Riesgo | Manejo implementado |
|---|---|
| Archivo CSV inexistente | `if (!archivo.is_open())` y mensaje de error |
| Base de datos vacía | `main.cpp` detiene la ejecución con mensaje claro |
| Entrada no numérica en menú | `leerOpcion()` usa `try/catch` con `std::invalid_argument` |
| Número fuera de rango | `leerOpcion()` vuelve a pedir la opción |
| Número demasiado grande | Captura `std::out_of_range` |
| Texto vacío | `leerTexto()` obliga a ingresar texto válido |
| Año no convertible | `RankingPorAnio` usa `try/catch` con `stoi()` |
| Valores desconocidos | Normalización a `unknown` |
| Filas corruptas o incompletas | Se completan campos faltantes |
| Duplicados en el árbol | `_anotar()` verifica antes de insertar ID |
| Sin resultados por árbol | Fallback paralelo en sinopsis |

Ejemplo de lectura segura:

```cpp
try {
    std::size_t pos;
    int valor = std::stoi(linea, &pos);

    if (pos != linea.size()) {
        std::cout << "Entrada invalida. Solo numeros enteros.\n";
        continue;
    }
} catch (const std::invalid_argument&) {
    std::cout << "Entrada invalida. Ingrese un numero.\n";
} catch (const std::out_of_range&) {
    std::cout << "Numero demasiado grande. Intente de nuevo.\n";
}
```

---

## Uso de la STL y paradigmas de C++

El proyecto utiliza de forma intensiva la librería estándar de C++ para mejorar organización, rendimiento y seguridad.

### Estructuras y utilidades utilizadas

| Elemento STL | Uso dentro del proyecto |
|---|---|
| `std::vector` | Listas de películas, tokens, IDs, likes y resultados |
| `std::unordered_map` | Catálogo de películas y nodos hijos del árbol |
| `std::shared_ptr` | Manejo de nodos del árbol |
| `std::unique_ptr` | Polimorfismo en buscadores creados por Factory |
| `std::sort` | Ordenamiento de resultados e IDs |
| `std::unique` | Eliminación de duplicados |
| `std::find` | Verificación de IDs repetidos |
| `std::shuffle` | Recomendaciones aleatorias |
| `std::transform` | Conversión de texto a minúsculas |
| `std::async` | Ejecución de tareas paralelas |
| `std::future` | Recuperación de resultados de tareas paralelas |
| `std::chrono` | Medición de tiempos |
| `std::istringstream` | Tokenización de texto |

### Paradigmas aplicados

| Paradigma | Evidencia |
|---|---|
| Programación Orientada a Objetos | `Pelicula`, `Buscador`, `RankingStrategy`, `ComandoUsuario`, `ResultadosPaginador` |
| Programación Genérica | `template<typename T> class SuffixTree` |
| Programación Paralela | `std::async`, `std::future`, división por chunks |
| Programación Modular | Separación en headers especializados |
| Polimorfismo | Interfaces `Buscador` y `RankingStrategy` |
| Reutilización | Funciones `limpiar()`, `tokenizar()`, `ordenarConEstrategia()` |

---

## Interfaz y autonomía del programa

La interfaz es por consola, pero se diseñó para ser autónoma e intuitiva.

```txt
========================================
                NETFLIX
========================================
  [1] Buscar pelicula
  [2] Ver mas tarde
  [3] Peliculas que le di like
  [4] Ver pelicula recomendada
  [0] Salir
----------------------------------------
```

### Flujo de uso

```txt
Usuario entra al menú
        ↓
Elige buscar por palabra o tag
        ↓
Escribe consulta
        ↓
Sistema muestra resultados de 5 en 5
        ↓
Usuario abre una película
        ↓
Puede dar Like o guardar en Ver más tarde
        ↓
Sistema actualiza recomendaciones
```

### Opciones disponibles

| Opción | Descripción |
|---|---|
| Buscar película | Permite buscar por palabra clave o tag |
| Ver más tarde | Muestra películas guardadas por el usuario |
| Películas con Like | Muestra películas marcadas con Like |
| Película recomendada | Muestra recomendaciones según likes o aleatorias |
| Salir | Finaliza el programa |

---

## Algoritmo de recomendaciones

El sistema implementa un algoritmo propio de recomendación basado en los likes del usuario.

```txt
INICIO

1. Obtener películas base:
   - likes de la sesión actual
   - si no hay, cargar historial desde likes.txt

2. Buscar candidatas que compartan al menos 2 géneros.

3. Si no hay candidatas, buscar mismo actor o mismo director.

4. Si no hay likes o coincidencias, mostrar películas aleatorias.

5. Devolver máximo 5 recomendaciones.

FIN
```

| Caso | Recomendación generada |
|---|---|
| Usuario tiene likes | Películas con géneros similares |
| No hay 2 géneros en común | Películas con mismo actor o director |
| No hay historial | Películas aleatorias |
| IDs antiguos no existen | Películas aleatorias |

---

## Checklist de rúbrica

| Criterio de evaluación | Evidencia en el proyecto |
|---|---|
| No tiene errores aparentes | Validaciones de archivos, entradas, rangos, datos incompletos y conversiones |
| Funcionamiento eficiente con gran volumen de información | Dataset de 34,886 películas, árbol de sufijos, `unordered_map`, deduplicación y fallback paralelo |
| Buen control de errores y excepciones | `try/catch`, validación de CSV, normalización de `unknown`, fallback de búsqueda |
| Alto grado de autonomía | Limpia datos, carga catálogo, indexa, busca, muestra detalles y recomienda sin intervención manual |
| Interfaz intuitiva | Menú numerado, resultados paginados, opciones claras |
| Documentación adecuada | README con arquitectura, algoritmos, complejidad, patrones y bibliografía |
| Buen nivel de abstracción | Separación en `MotorBusqueda`, `SuffixTree`, `Vistas`, `Consoleutils`, `limpiezadatos` |
| Organización en librerías | Uso de headers especializados por responsabilidad |
| Uso de clases, enums y funciones | `Pelicula`, `TipoTag`, `Buscador`, `RankingStrategy`, `ResultadosPaginador` |
| POO | Herencia, polimorfismo, encapsulamiento y destructores virtuales |
| Programación Genérica | `SuffixTree<T>` |
| Programación Paralela | Limpieza, carga y fallback con `std::async` y `std::future` |
| Uso adecuado de STL | `vector`, `unordered_map`, `sort`, `unique`, `find`, `shuffle`, `transform` |
| Eficiencia de tiempo y espacio | Árbol guarda IDs, no objetos; sinopsis se indexa por palabra completa para controlar memoria |
| Algoritmos estándar usados correctamente | Ordenamiento, deduplicación, búsqueda, transformación y aleatorización con STL |
| Al menos 4 patrones de diseño | Strategy, Factory Method, Iterator, Command |
| Tabla comparativa de tiempos | Incluida en la sección de rendimiento |
| Uso de GitHub | Repositorio con código, dataset, documentación y recursos del proyecto |
| Citas bibliográficas en APA | Incluidas en la sección final |
| Presentación de impacto | El README resume puntos clave para explicar estructura, rendimiento y patrones |

---

## Recomendaciones para la exposición

Para explicar el proyecto de forma clara, se recomienda dividir la exposición en este orden:

| Parte | Qué explicar |
|---|---|
| 1. Problema | Búsqueda eficiente en un CSV grande de películas |
| 2. Dataset | 34,886 películas, campos principales y limpieza necesaria |
| 3. Estructura de datos | Árbol de sufijos comprimido y motivo de elección |
| 4. Búsqueda | Palabra, frase, sub-palabra y tags |
| 5. Patrones | Strategy, Factory, Iterator y Command |
| 6. Paralelismo | Limpieza, carga y fallback de sinopsis |
| 7. Rendimiento | Comparación de tiempos y explicación de mejoras |
| 8. Demo | Buscar una película, abrir detalle, dar like y ver recomendación |

---

## Fuentes bibliográficas formato APA

- Cormen, T. H., Leiserson, C. E., Rivest, R. L., & Stein, C. (2022). *Introduction to Algorithms* (4th ed.). MIT Press.
- Gamma, E., Helm, R., Johnson, R., & Vlissides, J. (1994). *Design Patterns: Elements of Reusable Object-Oriented Software*. Addison-Wesley.
- Gusfield, D. (1997). *Algorithms on Strings, Trees, and Sequences: Computer Science and Computational Biology*. Cambridge University Press.
- Robischon, J. (2018). *Wikipedia Movie Plots* [Data set]. Kaggle. https://www.kaggle.com/datasets/jrobischon/wikipedia-movie-plots
- cppreference.com. (s. f.). *std::async*. https://en.cppreference.com/w/cpp/thread/async
- cppreference.com. (s. f.). *std::future*. https://en.cppreference.com/w/cpp/thread/future
- cppreference.com. (s. f.). *std::thread::hardware_concurrency*. https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency

---

## Conclusión

El proyecto cumple con los objetivos principales de Programación III porque integra una estructura de datos especializada, programación orientada a objetos, programación genérica, programación paralela, uso intensivo de la STL y patrones de diseño. Además, el sistema trabaja con un volumen considerable de información y mantiene una interfaz de consola clara para el usuario.

La decisión más importante fue usar un `SuffixTree<int>` para recuperar candidatos rápidamente sin guardar películas completas dentro del árbol. Esto mejora la eficiencia espacial y permite que el motor de búsqueda sea más escalable, manteniendo al mismo tiempo funcionalidades de usuario como paginación, likes, ver más tarde y recomendaciones.

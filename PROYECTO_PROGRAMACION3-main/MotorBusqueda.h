#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <unordered_map>
#include <future>
#include <chrono>
#include <thread>
#include <memory>
#include <random>
#include <cstdlib>

#include "SuffixTree.h"

using namespace std;
using namespace std::chrono;

// ── Índices de columnas del CSV de Wikipedia ──────────────────────────────────
static constexpr int COL_ANIO      = 0;
static constexpr int COL_TITULO    = 1;
static constexpr int COL_DIRECTOR  = 3;
static constexpr int COL_CAST      = 4;
static constexpr int COL_GENERO    = 5;
static constexpr int COL_SINOPSIS  = 7;


// ── Estructura de película ────────────────────────────────────────────────────
struct Pelicula {
    int id;
    string anio;
    string titulo;
    string sinopsis;
    vector<string> directores;
    vector<string> actores;
    vector<string> generos;
};


// ── Parsear una línea CSV respetando comas dentro de comillas ─────────────────
inline vector<string> parsearLinea(const string& linea) {
    vector<string> fila;
    string celda;
    bool en_comillas = false;

    for (size_t i = 0; i < linea.size(); i++) {
        char c = linea[i];

        if (c == '"') {
            // Comilla doble escapada ("") dentro de campo
            if (en_comillas && i + 1 < linea.size() && linea[i + 1] == '"') {
                celda += '"';
                i++;
            } else {
                en_comillas = !en_comillas;
            }
        } else if (c == ',' && !en_comillas) {
            fila.push_back(celda);
            celda.clear();
        } else {
            celda += c;
        }
    }

    fila.push_back(celda);
    return fila;
}


// ── Divide campos separados por coma: directores, actores, géneros ────────────
inline vector<string> dividir(string& texto) {
    vector<string> fila;
    string celda;

    if (texto.size() >= 2 && texto.front() == '"' && texto.back() == '"') {
        texto = texto.substr(1, texto.size() - 2);
    }

    for (char c : texto) {
        if (c == ',') {
            size_t ini = celda.find_first_not_of(" \t");
            size_t fin = celda.find_last_not_of(" \t");

            if (ini != string::npos) {
                fila.push_back(celda.substr(ini, fin - ini + 1));
            } else {
                fila.push_back("unknown");
            }

            celda.clear();
        } else {
            celda += c;
        }
    }

    size_t ini = celda.find_first_not_of(" \t");
    size_t fin = celda.find_last_not_of(" \t");

    if (ini != string::npos) {
        fila.push_back(celda.substr(ini, fin - ini + 1));
    } else if (!celda.empty()) {
        fila.push_back("unknown");
    }

    return fila;
}


// ── Limpieza: minúsculas + eliminar puntuación especial ───────────────────────
inline string limpiar(string texto) {
    transform(texto.begin(), texto.end(), texto.begin(),
              [](unsigned char c) {
                  return tolower(c);
              });

    const string descartados = ".,()!?;:\"'\\-_/|[]{}*#@^~`+=<>%";
    string resultado;
    resultado.reserve(texto.size());

    for (char c : texto) {
        if (descartados.find(c) == string::npos) {
            resultado += c;
        }
    }

    return resultado;
}


// ── Tokenización: divide string limpio en palabras ────────────────────────────
inline vector<string> tokenizar(string texto) {
    texto = limpiar(texto);

    vector<string> tokens;
    istringstream ss(texto);
    string palabra;

    while (ss >> palabra) {
        if (!palabra.empty()) {
            tokens.push_back(palabra);
        }
    }

    return tokens;
}


// ── Detecta si una línea empieza con un año: 4 dígitos + coma ─────────────────
inline bool esInicioNuevaPelicula(const string& linea) {
    if (linea.size() < 5) {
        return false;
    }

    return isdigit(linea[0]) &&
           isdigit(linea[1]) &&
           isdigit(linea[2]) &&
           isdigit(linea[3]) &&
           linea[4] == ',';
}


// ── Carga el CSV y devuelve unordered_map<int, Pelicula> ──────────────────────
// Programación paralela: divide las líneas completas entre varios hilos.
inline unordered_map<int, Pelicula> cargarCSV(const string& ruta = "wiki_movie_plots_deduped_final.csv") {
    auto inic = high_resolution_clock::now();

    unordered_map<int, Pelicula> catalogo;
    ifstream archivo(ruta);

    if (!archivo.is_open()) {
        cerr << "[MotorBusqueda] Error: no se pudo abrir \"" << ruta << "\"\n";
        return catalogo;
    }

    string linea;
    bool primeraLinea = true;
    vector<string> lineasCompletas;

    while (getline(archivo, linea)) {
        if (primeraLinea) {
            primeraLinea = false;
            continue;
        }

        if (linea.empty()) {
            continue;
        }

        string lineaCompleta = linea;

        while (true) {
            streampos posicion = archivo.tellg();
            string siguiente;

            if (!getline(archivo, siguiente)) {
                break;
            }

            if (esInicioNuevaPelicula(siguiente)) {
                archivo.seekg(posicion);
                break;
            }

            lineaCompleta += "\n" + siguiente;
        }

        lineasCompletas.push_back(move(lineaCompleta));
    }

    archivo.close();

    unsigned int numHilos = thread::hardware_concurrency();

    if (numHilos == 0) {
        numHilos = 4;
    }

    size_t totalPeliculas = lineasCompletas.size();

    if (totalPeliculas == 0) {
        return catalogo;
    }

    numHilos = min<unsigned int>(numHilos, totalPeliculas);

    size_t tamanoChunk = totalPeliculas / numHilos;
    size_t resto = totalPeliculas % numHilos;

    vector<future<vector<Pelicula>>> futuros;

    size_t inicio = 0;

    for (unsigned int i = 0; i < numHilos; ++i) {
        size_t fin = inicio + tamanoChunk + (i < resto ? 1 : 0);

        futuros.push_back(async(launch::async, [inicio, fin, &lineasCompletas]() {
            vector<Pelicula> peliculasLocales;
            peliculasLocales.reserve(fin - inicio);

            for (size_t j = inicio; j < fin; ++j) {
                vector<string> fila = parsearLinea(lineasCompletas[j]);

                // Ya no se descartan filas: limpiezadatos.h garantiza que
                // cada fila tenga sus 8 columnas (rellenando con "" lo que
                // falte), asi que aca solo queda el respaldo defensivo por
                // si algo llegara igual con menos columnas de las esperadas.
                Pelicula p;

                p.id       = static_cast<int>(j);
                p.anio     = fila.size() > COL_ANIO     ? fila[COL_ANIO]     : "unknown";
                p.titulo   = fila.size() > COL_TITULO   ? fila[COL_TITULO]   : "unknown";
                p.sinopsis = fila.size() > COL_SINOPSIS ? fila[COL_SINOPSIS] : "unknown";

                string director = fila.size() > COL_DIRECTOR ? fila[COL_DIRECTOR] : "unknown";
                string cast     = fila.size() > COL_CAST     ? fila[COL_CAST]     : "unknown";
                string genero   = fila.size() > COL_GENERO   ? fila[COL_GENERO]   : "unknown";

                p.directores = director != "unknown" ? dividir(director) : vector<string>{"unknown"};
                p.actores    = cast     != "unknown" ? dividir(cast)     : vector<string>{"unknown"};
                p.generos    = genero   != "unknown" ? dividir(genero)   : vector<string>{"unknown"};

                peliculasLocales.push_back(move(p));
            }

            return peliculasLocales;
        }));

        inicio = fin;
    }

    for (auto& f : futuros) {
        vector<Pelicula> subLista = f.get();

        for (auto& p : subLista) {
            catalogo[p.id] = move(p);
        }
    }

    cout << "[MotorBusqueda] " << catalogo.size()
         << " peliculas cargadas EN PARALELO desde \"" << ruta << "\"\n";

    auto fin = high_resolution_clock::now();
    auto duracion = duration_cast<milliseconds>(fin - inic);

    cout << "Tiempo: " << duracion.count() << " ms" << endl;

    return catalogo;
}


// ── Búsqueda por token en título y sinopsis ───────────────────────────────────
// Esta búsqueda queda como alternativa por recorrido paralelo.
inline vector<Pelicula> buscarPorPalabra(
    const unordered_map<int, Pelicula>& catalogo,
    const string& consulta
) {
    auto inic = high_resolution_clock::now();

    vector<string> tokensConsulta = tokenizar(consulta);
    vector<const Pelicula*> todasLasPeliculas;

    for (const auto& par : catalogo) {
        todasLasPeliculas.push_back(&par.second);
    }

    if (todasLasPeliculas.empty()) {
        return {};
    }

    unsigned int numHilos = thread::hardware_concurrency();

    if (numHilos == 0) {
        numHilos = 4;
    }

    numHilos = min<unsigned int>(numHilos, todasLasPeliculas.size());

    size_t tamanoChunk = todasLasPeliculas.size() / numHilos;
    size_t resto = todasLasPeliculas.size() % numHilos;

    vector<future<vector<Pelicula>>> futuros;

    size_t inicio = 0;

    for (unsigned int i = 0; i < numHilos; ++i) {
        size_t fin = inicio + tamanoChunk + (i < resto ? 1 : 0);

        futuros.push_back(async(launch::async,
            [inicio, fin, &todasLasPeliculas, &tokensConsulta]() {
                vector<Pelicula> resultadosLocales;

                for (size_t j = inicio; j < fin; ++j) {
                    const Pelicula& p = *todasLasPeliculas[j];
                    bool encontrado = false;

                    vector<string> tokensTitulo = tokenizar(p.titulo);

                    for (const auto& palabra : tokensTitulo) {
                        for (const auto& token : tokensConsulta) {
                            if (palabra == token) {
                                resultadosLocales.push_back(p);
                                encontrado = true;
                                break;
                            }
                        }

                        if (encontrado) {
                            break;
                        }
                    }

                    if (encontrado) {
                        continue;
                    }

                    vector<string> tokensSinopsis = tokenizar(p.sinopsis);

                    for (const auto& palabra : tokensSinopsis) {
                        for (const auto& token : tokensConsulta) {
                            if (palabra == token) {
                                resultadosLocales.push_back(p);
                                encontrado = true;
                                break;
                            }
                        }

                        if (encontrado) {
                            break;
                        }
                    }
                }

                return resultadosLocales;
            }
        ));

        inicio = fin;
    }

    vector<Pelicula> resultados;

    for (auto& futuro : futuros) {
        vector<Pelicula> parciales = futuro.get();
        resultados.insert(resultados.end(), parciales.begin(), parciales.end());
    }

    auto fin = high_resolution_clock::now();
    auto duracion = duration_cast<milliseconds>(fin - inic);

    cout << "Tiempo: " << duracion.count() << " ms" << endl;

    return resultados;
}


// Declaracion adelantada: la definicion completa esta mas abajo, junto al
// resto de las funciones que usan el Arbol de Sufijos como filtro rapido de
// candidatos. Se declara aca arriba porque buscarPorTag() (justo debajo) ya
// la necesita.
inline vector<int> idsCandidatosPorSuffixTree(
    const SuffixTree<int>& arbol,
    const vector<string>& tokensConsulta
);

// ── Tipos de tag disponibles para la búsqueda por etiqueta ─────────────────────
// Antes buscarPorTag mezclaba director/actor/genero en un solo criterio
// "generico" y probaba los tres en cascada. Ahora el usuario elige de
// antemano CUAL campo quiere consultar (Director, Actor, Genero o Anio) y la
// busqueda se hace unicamente sobre ese campo, evitando falsos positivos
// (ej. buscar "accion" y que combine con un director que se llame igual).
enum class TipoTag {
    DIRECTOR,
    ACTOR,
    GENERO,
    ANIO
};

// ── Helper: compara los tokens de un campo de la pelicula contra los tokens
// de la consulta del usuario. Coincidencia exacta de palabra (no substring),
// igual criterio que se usaba antes en buscarPorTag. ─────────────────────────
inline bool coincideCampoTag(const vector<string>& tokensCampo, const vector<string>& tokensConsulta) {
    for (const auto& palabra : tokensCampo) {
        for (const auto& token : tokensConsulta) {
            if (palabra == token) {
                return true;
            }
        }
    }
    return false;
}

// ── Búsqueda por tag: director, actor, género o año ────────────────────────────
// A diferencia de la version anterior (que probaba director -> actor ->
// genero en cascada sobre el mismo texto), ahora el tipo de tag se elige
// explicitamente en la vista y se busca UNICAMENTE en ese campo.
//
// Igual que en buscarPorTextoConPrioridad, el arbol se usa primero como
// filtro rapido de candidatos (idsCandidatosPorSuffixTree). El arbol indexa
// director/actor/genero mezclados con titulo y sinopsis en el mismo set de
// IDs, asi que los candidatos que devuelve son un SUPERCONJUNTO de la
// respuesta correcta (pueden venir ids que matchean por otro campo). Por
// eso, igual que antes, se verifica el campo especifico (coincideCampoTag)
// -la diferencia es que ahora esa verificacion se hace sobre el
// subconjunto de candidatos del arbol, no sobre las ~34000 peliculas del
// catalogo completo.
inline vector<Pelicula> buscarPorTag(
    const unordered_map<int, Pelicula>& catalogo,
    const SuffixTree<int>& arbol,
    const string& consulta,
    TipoTag tipo
) {
    vector<string> tokensConsulta = tokenizar(consulta);
    vector<Pelicula> resultados;

    if (tokensConsulta.empty()) {
        return resultados;
    }

    vector<int> idsCandidatos = idsCandidatosPorSuffixTree(arbol, tokensConsulta);

    vector<const Pelicula*> candidatos;
    candidatos.reserve(idsCandidatos.size());
    for (int id : idsCandidatos) {
        auto it = catalogo.find(id);
        if (it != catalogo.end()) candidatos.push_back(&it->second);
    }

    // Resguardo: si por algun motivo el arbol no devolvio candidatos (por
    // ejemplo, el catalogo esta vacio o el token es demasiado corto), se
    // cae al recorrido completo para no perder resultados validos.
    if (candidatos.empty()) {
        for (const auto& par : catalogo) candidatos.push_back(&par.second);
    }

    for (const Pelicula* pPtr : candidatos) {
        const Pelicula& p = *pPtr;
        bool encontrado = false;

        switch (tipo) {
            case TipoTag::DIRECTOR:
                for (const auto& director : p.directores) {
                    if (coincideCampoTag(tokenizar(director), tokensConsulta)) {
                        encontrado = true;
                        break;
                    }
                }
                break;

            case TipoTag::ACTOR:
                for (const auto& actor : p.actores) {
                    if (coincideCampoTag(tokenizar(actor), tokensConsulta)) {
                        encontrado = true;
                        break;
                    }
                }
                break;

            case TipoTag::GENERO:
                for (const auto& genero : p.generos) {
                    if (coincideCampoTag(tokenizar(genero), tokensConsulta)) {
                        encontrado = true;
                        break;
                    }
                }
                break;

            case TipoTag::ANIO:
                // El anio es un campo unico (no una lista), se compara igual.
                if (coincideCampoTag(tokenizar(p.anio), tokensConsulta)) {
                    encontrado = true;
                }
                break;
        }

        if (encontrado) {
            resultados.push_back(p);
        }
    }

    return resultados;
}


// ─────────────────────────────────────────────────────────────────────────────
// PATRÓN STRATEGY
// RankingStrategy permite cambiar el algoritmo de importancia sin tocar top5.
// ─────────────────────────────────────────────────────────────────────────────

class RankingStrategy {
public:
    virtual int calcularScore(const Pelicula& p, const string& consulta) const = 0;
    virtual ~RankingStrategy() = default;
};


class RankingPorRelevancia : public RankingStrategy {
public:
    int calcularScore(const Pelicula& p, const string& consulta) const override {
        vector<string> tokens = tokenizar(consulta);
        int score = 0;

        // Título — peso 100
        for (const auto& t : tokenizar(p.titulo)) {
            for (const auto& token : tokens) {
                if (t.find(token) != string::npos) {
                    score += 100;
                }
            }
        }

        // Géneros — peso 50
        for (const auto& genero : p.generos) {
            for (const auto& palabra : tokenizar(genero)) {
                for (const auto& token : tokens) {
                    if (palabra.find(token) != string::npos) {
                        score += 50;
                    }
                }
            }
        }

        // Directores — peso 10
        for (const auto& d : p.directores) {
            for (const auto& t : tokenizar(d)) {
                for (const auto& token : tokens) {
                    if (t.find(token) != string::npos) {
                        score += 10;
                    }
                }
            }
        }

        // Actores — peso 10
        for (const auto& a : p.actores) {
            for (const auto& t : tokenizar(a)) {
                for (const auto& token : tokens) {
                    if (t.find(token) != string::npos) {
                        score += 10;
                    }
                }
            }
        }

        // Anio — peso 30 (relevante sobre todo para busqueda por tag de anio)
        for (const auto& t : tokenizar(p.anio)) {
            for (const auto& token : tokens) {
                if (t == token) {
                    score += 30;
                }
            }
        }

        // Sinopsis — peso 1
        for (const auto& t : tokenizar(p.sinopsis)) {
            for (const auto& token : tokens) {
                if (t.find(token) != string::npos) {
                    score += 1;
                }
            }
        }

        return score;
    }
};


// ── SEGUNDA ESTRATEGIA CONCRETA ────────────────────────────────────────────────
// Ordena priorizando el AÑO: primero coincidencia exacta de año con la
// consulta (si la consulta trae un numero de 4 digitos), y como criterio de
// desempate, peliculas mas recientes primero. Pensada especialmente para
// TipoTag::ANIO, donde "relevancia por texto" no tiene mucho sentido (el
// campo Anio es un numero, no texto libre) pero "que tan cerca esta del
// anio buscado" si. Esta es la prueba de que RankingStrategy realmente se
// puede intercambiar sin tocar el codigo que la usa (ordenarConEstrategia).
class RankingPorAnio : public RankingStrategy {
public:
    int calcularScore(const Pelicula& p, const string& consulta) const override {
        int score = 0;

        int anioPelicula = 0;
        try {
            anioPelicula = stoi(p.anio);
        } catch (...) {
            return 0; // anio no numerico ("unknown", etc): al final del orden
        }

        for (const auto& token : tokenizar(consulta)) {
            try {
                int anioBuscado = stoi(token);

                if (anioPelicula == anioBuscado) {
                    score += 1000; // coincidencia exacta de anio: maxima prioridad
                } else {
                    // Mientras mas cerca este del anio buscado, mayor score.
                    int distancia = abs(anioPelicula - anioBuscado);
                    score += max(0, 200 - distancia);
                }
            } catch (...) {
                // token no numerico: no aporta a este criterio
            }
        }

        // Desempate leve: peliculas mas recientes primero.
        score += anioPelicula / 100;

        return score;
    }
};


// ── Ranking de resultados: ordena TODOS por relevancia usando Strategy ────────
// (antes 'top5' recortaba a 5 aca mismo, lo cual le quitaba a la vista la
// posibilidad de paginar mas alla de esos 5. Ahora el corte/paginado es
// responsabilidad de la vista -> ver ResultadosPaginador en Vistas.h)
//
// PATRON STRATEGY EN ACCION: esta funcion no sabe (ni le importa) que
// estrategia concreta le pasaron; solo llama a calcularScore() de forma
// polimorfica. Asi se puede pasar RankingPorRelevancia, RankingPorAnio, o
// cualquier otra estrategia futura, sin cambiar una linea de este codigo.
inline vector<Pelicula> ordenarConEstrategia(
    const vector<Pelicula>& resultados,
    const string& consulta,
    const RankingStrategy& estrategia
) {
    vector<pair<int, Pelicula>> conteo;

    for (const auto& p : resultados) {
        int score = estrategia.calcularScore(p, consulta);
        conteo.push_back({score, p});
    }

    sort(conteo.begin(), conteo.end(),
         [](const pair<int, Pelicula>& a, const pair<int, Pelicula>& b) {
             return a.first > b.first;
         });

    vector<Pelicula> resultado;
    resultado.reserve(conteo.size());
    for (auto& par : conteo) {
        resultado.push_back(move(par.second));
    }

    return resultado;
}

// Se mantiene la firma original por compatibilidad: usa RankingPorRelevancia
// como estrategia por defecto.
inline vector<Pelicula> ordenarPorRelevancia(
    const vector<Pelicula>& resultados,
    const string& consulta
) {
    RankingPorRelevancia estrategia;
    return ordenarConEstrategia(resultados, consulta, estrategia);
}

// Se mantiene por compatibilidad si algo todavia espera solo los primeros 5.
inline vector<Pelicula> top5(
    const vector<Pelicula>& resultados,
    const string& consulta
) {
    vector<Pelicula> ordenados = ordenarPorRelevancia(resultados, consulta);
    int limite = min(5, static_cast<int>(ordenados.size()));
    return vector<Pelicula>(ordenados.begin(), ordenados.begin() + limite);
}


// ─────────────────────────────────────────────────────────────────────────────
// PERSISTENCIA DE LIKES
// Cada vez que el usuario da/quita like, el vector 'likes' en memoria se
// vuelca completo a un .txt (un id por linea). Asi, si en una sesion futura
// el usuario todavia no dio ningun like, se puede recuperar el historial de
// la sesion anterior para generar recomendaciones.
// ─────────────────────────────────────────────────────────────────────────────
static const string ARCHIVO_LIKES = "likes.txt";

inline vector<int> cargarLikesDesdeArchivo(const string& ruta = ARCHIVO_LIKES) {
    vector<int> ids;
    ifstream archivo(ruta);

    if (!archivo.is_open()) {
        return ids;
    }

    int id;
    while (archivo >> id) {
        ids.push_back(id);
    }

    return ids;
}

inline void guardarLikesEnArchivo(const vector<int>& likes, const string& ruta = ARCHIVO_LIKES) {
    ofstream archivo(ruta, ios::trunc);

    if (!archivo.is_open()) {
        return;
    }

    for (int id : likes) {
        archivo << id << "\n";
    }
}


// ─────────────────────────────────────────────────────────────────────────────
// RECOMENDACIONES
// Algoritmo propio para "Peliculas que te pueden interesar" / "Recomendada":
//
//   1) Se determina la lista BASE de peliculas que le gustan al usuario:
//        - Si hay likes en la sesion actual, se usan esos.
//        - Si no hay likes en la sesion actual, se recurre al historial
//          guardado en likes.txt (likes de sesiones anteriores).
//   2) Con esa base, se buscan candidatas (que no sean ellas mismas) que
//      compartan AL MENOS 2 generos con alguna pelicula base.
//   3) Si ninguna candidata cumple el paso 2, se buscan candidatas que
//      compartan el mismo actor O el mismo director con alguna base.
//   4) Si tampoco hay coincidencias, o si nunca hubo ningun like (ni en la
//      sesion actual ni en el historial), se muestran peliculas aleatorias.
//   En todos los casos el resultado se limita a un TOP 5.
// ─────────────────────────────────────────────────────────────────────────────
inline vector<Pelicula> peliculasAleatorias(
    const unordered_map<int, Pelicula>& db,
    int cantidad = 5
) {
    vector<Pelicula> todas;
    todas.reserve(db.size());

    for (const auto& par : db) {
        todas.push_back(par.second);
    }

    static std::mt19937 generadorAleatorio(std::random_device{}());
    std::shuffle(todas.begin(), todas.end(), generadorAleatorio);

    int limite = min(cantidad, static_cast<int>(todas.size()));
    return vector<Pelicula>(todas.begin(), todas.begin() + limite);
}

inline vector<Pelicula> obtenerPeliculasRecomendadas(
    const unordered_map<int, Pelicula>& db,
    const vector<int>& likesSesion
) {
    // 1) Determinar la base: likes de la sesion actual, o historial en disco.
    vector<int> idsBase = likesSesion;

    if (idsBase.empty()) {
        idsBase = cargarLikesDesdeArchivo();
    }

    if (idsBase.empty()) {
        // Nunca hubo ningun like: peliculas aleatorias.
        return peliculasAleatorias(db, 5);
    }

    vector<Pelicula> baseMovies;
    for (int id : idsBase) {
        auto it = db.find(id);
        if (it != db.end()) {
            baseMovies.push_back(it->second);
        }
    }

    if (baseMovies.empty()) {
        // Los ids guardados ya no existen en la base de datos actual.
        return peliculasAleatorias(db, 5);
    }

    auto esBase = [&idsBase](int id) {
        return find(idsBase.begin(), idsBase.end(), id) != idsBase.end();
    };

    // 2) Candidatas con al menos 2 generos en comun con alguna base.
    vector<pair<int, Pelicula>> candidatosGenero; // (cantidad de generos en comun, pelicula)

    for (const auto& par : db) {
        const Pelicula& candidata = par.second;

        if (esBase(candidata.id)) {
            continue;
        }

        int mejorCoincidencia = 0;

        for (const auto& base : baseMovies) {
            int coincidencias = 0;

            for (const auto& gBase : base.generos) {
                string gBaseLimpio = limpiar(gBase);
                if (gBaseLimpio == "unknown") continue;

                for (const auto& gCand : candidata.generos) {
                    if (gBaseLimpio == limpiar(gCand)) {
                        coincidencias++;
                    }
                }
            }

            mejorCoincidencia = max(mejorCoincidencia, coincidencias);
        }

        if (mejorCoincidencia >= 2) {
            candidatosGenero.push_back({mejorCoincidencia, candidata});
        }
    }

    if (!candidatosGenero.empty()) {
        sort(candidatosGenero.begin(), candidatosGenero.end(),
             [](const pair<int, Pelicula>& a, const pair<int, Pelicula>& b) {
                 return a.first > b.first;
             });

        vector<Pelicula> resultado;
        for (auto& par : candidatosGenero) {
            resultado.push_back(move(par.second));
            if (resultado.size() == 5) break;
        }
        return resultado;
    }

    // 3) Sin coincidencia de 2+ generos: buscar mismo actor o mismo director.
    vector<Pelicula> candidatosActorDirector;

    for (const auto& par : db) {
        const Pelicula& candidata = par.second;

        if (esBase(candidata.id)) {
            continue;
        }

        bool coincide = false;

        for (const auto& base : baseMovies) {
            for (const auto& dBase : base.directores) {
                string dBaseLimpio = limpiar(dBase);
                if (dBaseLimpio == "unknown") continue;

                for (const auto& dCand : candidata.directores) {
                    if (dBaseLimpio == limpiar(dCand)) {
                        coincide = true;
                        break;
                    }
                }
                if (coincide) break;
            }
            if (coincide) break;

            for (const auto& aBase : base.actores) {
                string aBaseLimpio = limpiar(aBase);
                if (aBaseLimpio == "unknown") continue;

                for (const auto& aCand : candidata.actores) {
                    if (aBaseLimpio == limpiar(aCand)) {
                        coincide = true;
                        break;
                    }
                }
                if (coincide) break;
            }
            if (coincide) break;
        }

        if (coincide) {
            candidatosActorDirector.push_back(candidata);
            if (candidatosActorDirector.size() == 5) break;
        }
    }

    return candidatosActorDirector;
}


// ── Indexación del catálogo en el árbol de sufijos ────────────────────────────
// IMPORTANTE: aca esta la diferencia entre "insertar" (inserta TODOS los
// sufijos de la palabra: para "barco" inserta barco/arco/rco/co/o) e
// "insertarPalabraCompleta" (inserta solo la palabra entera). Usar
// insertarPalabraCompleta en todos los campos (como estaba antes) hace que
// arbol.buscar("bar") NUNCA encuentre "barco", porque "bar" no es ninguna de
// las palabras completas indexadas: el arbol de sufijos termina
// funcionando, en la practica, como un simple diccionario de palabras
// exactas, no como un buscador de sub-cadenas.
//
// Por eso, titulo/genero/director/actor (campos cortos) se indexan con
// insertar() -> sufijos completos, para que buscar sub-cadenas como "bar"
// dentro de "barco" funcione de verdad a traves del arbol.
//
// La Sinopsis puede tener miles de palabras por pelicula; insertar todos
// los sufijos de cada palabra de la sinopsis de ~34000 peliculas dispara el
// uso de memoria (cada sufijo es un nodo/rama nuevo en el arbol). Por eso
// la sinopsis se sigue indexando por PALABRA COMPLETA (insertarPalabraCompleta):
// el arbol encuentra rapido coincidencias de palabra completa en la
// sinopsis, y el caso de sub-cadena suelta dentro de la sinopsis (ej. "bar"
// como parte de una palabra que NO es la buscada como palabra completa) se
// resuelve con un fallback en paralelo sobre el texto crudo, ver
// buscarSustringEnSinopsisParalelo() mas abajo. Esta es una decision de
// diseño de espacio-vs-cobertura y debe quedar documentada en el README.
inline void indexarCatalogo(
    const unordered_map<int, Pelicula>& catalogo,
    SuffixTree<int>& arbol
) {
    for (const auto& par : catalogo) {
        const Pelicula& p = par.second;

        // Campos de alta prioridad: se indexan TODOS los sufijos, asi el
        // arbol si soporta busqueda por sub-cadena real (no solo palabra
        // exacta) en titulo/genero/director/actor.
        for (const auto& t : tokenizar(p.titulo)) {
            arbol.insertar(t, p.id);
        }

        for (const auto& g : p.generos) {
            for (const auto& t : tokenizar(g)) {
                arbol.insertar(t, p.id);
            }
        }

        for (const auto& d : p.directores) {
            for (const auto& t : tokenizar(d)) {
                arbol.insertar(t, p.id);
            }
        }

        for (const auto& a : p.actores) {
            for (const auto& t : tokenizar(a)) {
                arbol.insertar(t, p.id);
            }
        }

        // Sinopsis: se inserta palabra completa para controlar memoria.
        // Esto mantiene el programa más estable con un CSV grande.
        for (const auto& t : tokenizar(p.sinopsis)) {
            arbol.insertarPalabraCompleta(t, p.id);
        }
    }
}


// ── Búsqueda eficiente usando el árbol de sufijos ─────────────────────────────
// Devuelve el conjunto de IDs candidatos para una consulta de varias
// palabras, usando el arbol de sufijos (rapido) en vez de recorrer TODO el
// catalogo. Semantica OR: un id entra si CUALQUIERA de los tokens de la
// consulta aparece como sub-cadena en algun campo indexado (titulo, genero,
// director, actor o palabra completa de sinopsis) de esa pelicula.
// Ejemplo: "barco fantasma" -> tokens ["barco","fantasma"] -> union de
// arbol.buscar("barco") y arbol.buscar("fantasma").
inline vector<int> idsCandidatosPorSuffixTree(
    const SuffixTree<int>& arbol,
    const vector<string>& tokensConsulta
) {
    vector<int> todosLosIds;

    for (const auto& token : tokensConsulta) {
        if (token.empty()) continue;
        vector<int> idsEncontrados = arbol.buscar(token);
        todosLosIds.insert(todosLosIds.end(), idsEncontrados.begin(), idsEncontrados.end());
    }

    sort(todosLosIds.begin(), todosLosIds.end());
    auto ultimoIdUnico = unique(todosLosIds.begin(), todosLosIds.end());
    todosLosIds.erase(ultimoIdUnico, todosLosIds.end());

    return todosLosIds;
}

// Version que ademas resuelve los IDs a Pelicula (queda disponible por si
// se necesita en algun otro lugar el resultado ya "materializado").
inline vector<Pelicula> buscarConSuffixTree(
    const SuffixTree<int>& arbol,
    const unordered_map<int, Pelicula>& catalogo,
    const string& consulta
) {
    vector<string> tokensConsulta = tokenizar(consulta);
    vector<Pelicula> resultados;

    if (tokensConsulta.empty()) {
        return resultados;
    }

    for (int id : idsCandidatosPorSuffixTree(arbol, tokensConsulta)) {
        auto it = catalogo.find(id);
        if (it != catalogo.end()) {
            resultados.push_back(it->second);
        }
    }

    return resultados;
}

// ── Fallback en PARALELO: sub-cadena suelta dentro de la Sinopsis ─────────────
// El arbol solo indexa la sinopsis por palabra COMPLETA (ver indexarCatalogo),
// asi que una sub-cadena que no es una palabra completa (ej. "bar" dentro de
// "embarcaron", que no es "barco") no aparece como candidata desde el arbol.
// Esta funcion cubre ese caso final recorriendo la sinopsis en paralelo y
// buscando cada token como sub-cadena literal. Solo se usa cuando la
// busqueda por arbol no encontro NADA, asi que el costo O(n) solo se paga en
// el peor caso, no en cada busqueda.
inline vector<Pelicula> buscarSustringEnSinopsisParalelo(
    const unordered_map<int, Pelicula>& catalogo,
    const vector<string>& tokensConsulta
) {
    vector<const Pelicula*> todas;
    todas.reserve(catalogo.size());
    for (const auto& par : catalogo) todas.push_back(&par.second);

    if (todas.empty()) return {};

    unsigned int numHilos = thread::hardware_concurrency();
    if (numHilos == 0) numHilos = 4;
    numHilos = min<unsigned int>(numHilos, todas.size());

    size_t tamanoChunk = todas.size() / numHilos;
    size_t resto = todas.size() % numHilos;

    vector<future<vector<Pelicula>>> futuros;
    size_t inicio = 0;

    for (unsigned int i = 0; i < numHilos; ++i) {
        size_t fin = inicio + tamanoChunk + (i < resto ? 1 : 0);

        futuros.push_back(async(launch::async, [inicio, fin, &todas, &tokensConsulta]() {
            vector<Pelicula> parciales;

            for (size_t j = inicio; j < fin; ++j) {
                const Pelicula& p = *todas[j];
                string sinopsisLimpia = limpiar(p.sinopsis);

                for (const auto& token : tokensConsulta) {
                    if (!token.empty() && sinopsisLimpia.find(token) != string::npos) {
                        parciales.push_back(p);
                        break;
                    }
                }
            }

            return parciales;
        }));

        inicio = fin;
    }

    vector<Pelicula> resultados;
    for (auto& f : futuros) {
        vector<Pelicula> parciales = f.get();
        resultados.insert(resultados.end(), parciales.begin(), parciales.end());
    }

    return resultados;
}


// ─────────────────────────────────────────────────────────────────────────────
// Búsqueda por palabra clave con prioridad de TÍTULO sobre SINOPSIS
// ─────────────────────────────────────────────────────────────────────────────
// Orden exigido:
//   1) Películas cuyo TÍTULO EMPIEZA con el texto buscado         (ej: "ba" -> "Barco...")
//   2) Películas cuyo TÍTULO CONTIENE el texto buscado en cualquier posicion
//      (ej: "ba" -> "El bar", "Submarino")
//   3) SOLO SI NINGÚN título (ni por 1 ni por 2) coincide en TODO el catálogo,
//      se busca el texto dentro de la SINOPSIS, y esos resultados se
//      devuelven en orden ALEATORIO (no hay "mas relevante" dentro de ellos).
// Se compara sobre texto "limpio" (minusculas, sin puntuacion) para que la
// comparacion sea insensible a mayusculas/acentos de puntuacion.
// ─────────────────────────────────────────────────────────────────────────────
// Búsqueda por palabra/frase/sub-cadena, con prioridad TÍTULO > SINOPSIS.
//
// Ahora SI usa el Arbol de Sufijos como primer filtro: en vez de recorrer
// las ~34000 peliculas del catalogo (O(n)) para clasificarlas, primero le
// pide al arbol los IDs candidatos (rapido, O(largo del patron) por token) y
// solo clasifica/ordena ESE subconjunto, ya mucho mas chico. Asi el arbol
// cumple su proposito real: acelerar la busqueda.
//
// Semantica exigida:
//   - Si se busca "barco"           -> cualquier pelicula con "barco" en
//                                       titulo o sinopsis (sub-cadena, no
//                                       necesita ser palabra completa).
//   - Si se busca "barco fantasma"  -> se tokeniza en ["barco","fantasma"] y
//                                       se devuelve la UNION (OR): peliculas
//                                       que tengan "barco" Y/O "fantasma".
//   - Si se busca "bar"             -> encuentra peliculas donde "bar" es
//                                       sub-cadena de una palabra mas larga
//                                       (ej. "barco"), no solo coincidencia
//                                       exacta de palabra.
//
// Orden de prioridad (igual que antes):
//   1) Titulo EMPIEZA con algun token de la consulta.
//   2) Titulo CONTIENE algun token en cualquier posicion.
//   3) Solo si NINGUN titulo coincidio: Sinopsis contiene algun token
//      (orden aleatorio dentro de este grupo, tal como se definio original).
inline vector<Pelicula> buscarPorTextoConPrioridad(
    const unordered_map<int, Pelicula>& catalogo,
    const SuffixTree<int>& arbol,
    const string& consultaOriginal
) {
    vector<string> tokensConsulta = tokenizar(consultaOriginal);

    if (tokensConsulta.empty()) {
        return {};
    }

    // ── PASO 1: candidatos rapidos via el arbol (union de todos los tokens) ──
    vector<int> idsCandidatos = idsCandidatosPorSuffixTree(arbol, tokensConsulta);

    vector<Pelicula> candidatos;
    candidatos.reserve(idsCandidatos.size());
    for (int id : idsCandidatos) {
        auto it = catalogo.find(id);
        if (it != catalogo.end()) candidatos.push_back(it->second);
    }

    // ── PASO 2: clasificar SOLO los candidatos (subconjunto chico) ───────────
    vector<Pelicula> empiezaConTitulo;
    vector<Pelicula> contieneEnTitulo;
    vector<Pelicula> coincidenSinopsis;

    for (const auto& p : candidatos) {
        string tituloLimpio = limpiar(p.titulo);
        string sinopsisLimpia = limpiar(p.sinopsis);

        bool tituloEmpieza = false;
        bool tituloContiene = false;
        bool sinopsisContiene = false;

        for (const auto& token : tokensConsulta) {
            if (token.empty()) continue;

            if (!tituloEmpieza && tituloLimpio.rfind(token, 0) == 0) {
                tituloEmpieza = true;
            }
            if (!tituloContiene && tituloLimpio.find(token) != string::npos) {
                tituloContiene = true;
            }
            if (!sinopsisContiene && sinopsisLimpia.find(token) != string::npos) {
                sinopsisContiene = true;
            }
        }

        if (tituloEmpieza) {
            empiezaConTitulo.push_back(p);
        } else if (tituloContiene) {
            contieneEnTitulo.push_back(p);
        } else if (sinopsisContiene) {
            coincidenSinopsis.push_back(p);
        }
    }

    if (!empiezaConTitulo.empty() || !contieneEnTitulo.empty()) {
        vector<Pelicula> resultado = move(empiezaConTitulo);
        resultado.insert(resultado.end(), contieneEnTitulo.begin(), contieneEnTitulo.end());
        return resultado;
    }

    if (!coincidenSinopsis.empty()) {
        static std::mt19937 generadorAleatorio(std::random_device{}());
        std::shuffle(coincidenSinopsis.begin(), coincidenSinopsis.end(), generadorAleatorio);
        return coincidenSinopsis;
    }

    // ── PASO 3: fallback O(n) en paralelo ─────────────────────────────────────
    // El arbol solo indexa la sinopsis por PALABRA COMPLETA (ver
    // indexarCatalogo). Si la busqueda no encontro nada por esa via, puede
    // ser una sub-cadena suelta dentro de la sinopsis (ej. "bar" dentro de
    // "embarcaron"). Este caso final se resuelve con un escaneo en paralelo,
    // que solo se paga cuando de verdad no hubo candidatos por el arbol.
    vector<Pelicula> porSustring = buscarSustringEnSinopsisParalelo(catalogo, tokensConsulta);

    static std::mt19937 generadorAleatorio2(std::random_device{}());
    std::shuffle(porSustring.begin(), porSustring.end(), generadorAleatorio2);

    return porSustring;
}


// ─────────────────────────────────────────────────────────────────────────────
// PATRÓN FACTORY METHOD
// BuscadorFactory crea el tipo de búsqueda sin poner if/else grandes en main.
// ─────────────────────────────────────────────────────────────────────────────

class Buscador {
public:
    virtual vector<Pelicula> buscar(
        const unordered_map<int, Pelicula>& db,
        const SuffixTree<int>& arbol,
        const string& consulta
    ) const = 0;

    virtual ~Buscador() = default;
};


class BuscadorPorTexto : public Buscador {
public:
    vector<Pelicula> buscar(
        const unordered_map<int, Pelicula>& db,
        const SuffixTree<int>& arbol,
        const string& consulta
    ) const override {
        // Prioridad titulo (empieza-con / contiene) > sinopsis (aleatorio).
        // Ahora SI usa el arbol como filtro rapido de candidatos: ver
        // buscarPorTextoConPrioridad para el detalle del criterio.
        return buscarPorTextoConPrioridad(db, arbol, consulta);
    }
};


class BuscadorPorTag : public Buscador {
private:
    TipoTag tipo;

public:
    explicit BuscadorPorTag(TipoTag tipo) : tipo(tipo) {}

    vector<Pelicula> buscar(
        const unordered_map<int, Pelicula>& db,
        const SuffixTree<int>& arbol,
        const string& consulta
    ) const override {
        // El arbol ahora se usa como pre-filtro de candidatos dentro de
        // buscarPorTag (ver comentario ahi), en vez de recorrer las ~34000
        // peliculas del catalogo completo.
        vector<Pelicula> resultados = buscarPorTag(db, arbol, consulta, tipo);

        // PATRON STRATEGY: la estrategia de ranking se elige segun el tipo
        // de tag. Para busqueda por Anio tiene mas sentido ordenar por
        // cercania/coincidencia de anio (RankingPorAnio) que por el
        // conteo de palabras de texto libre (RankingPorRelevancia), que es
        // la que se usa para Director/Actor/Genero. Ambas estrategias son
        // 100% intercambiables gracias a ordenarConEstrategia().
        if (tipo == TipoTag::ANIO) {
            RankingPorAnio estrategia;
            return ordenarConEstrategia(resultados, consulta, estrategia);
        }

        RankingPorRelevancia estrategia;
        return ordenarConEstrategia(resultados, consulta, estrategia);
    }
};


class BuscadorFactory {
public:
    // 'tipo' solo se usa cuando esBusquedaPorTag es true; para busqueda por
    // texto libre se ignora (BuscadorPorTexto usa el SuffixTree).
    static unique_ptr<Buscador> crearBuscador(bool esBusquedaPorTag, TipoTag tipo = TipoTag::GENERO) {
        if (esBusquedaPorTag) {
            return make_unique<BuscadorPorTag>(tipo);
        }

        return make_unique<BuscadorPorTexto>();
    }
};
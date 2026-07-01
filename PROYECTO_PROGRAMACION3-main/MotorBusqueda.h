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


// ── Búsqueda por tag: director, casting o género ──────────────────────────────
inline vector<Pelicula> buscarPorTag(
    const unordered_map<int, Pelicula>& catalogo,
    const string& consulta
) {
    vector<string> tokensConsulta = tokenizar(consulta);
    vector<Pelicula> resultados;

    for (const auto& par : catalogo) {
        const Pelicula& p = par.second;
        bool encontrado = false;

        // Buscar en directores
        for (const auto& director : p.directores) {
            vector<string> tokensDirector = tokenizar(director);

            for (const auto& palabra : tokensDirector) {
                for (const auto& token : tokensConsulta) {
                    if (palabra == token) {
                        resultados.push_back(p);
                        encontrado = true;
                        break;
                    }
                }

                if (encontrado) {
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

        // Buscar en actores / casting
        for (const auto& actor : p.actores) {
            vector<string> tokensActor = tokenizar(actor);

            for (const auto& palabra : tokensActor) {
                for (const auto& token : tokensConsulta) {
                    if (palabra == token) {
                        resultados.push_back(p);
                        encontrado = true;
                        break;
                    }
                }

                if (encontrado) {
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

        // Buscar en géneros
        for (const auto& genero : p.generos) {
            vector<string> tokensGenero = tokenizar(genero);

            for (const auto& palabra : tokensGenero) {
                for (const auto& token : tokensConsulta) {
                    if (palabra == token) {
                        resultados.push_back(p);
                        encontrado = true;
                        break;
                    }
                }

                if (encontrado) {
                    break;
                }
            }

            if (encontrado) {
                break;
            }
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


// ── Ranking de resultados: mantiene tu top5 actual, pero usando Strategy ──────
inline vector<Pelicula> top5(
    const vector<Pelicula>& resultados,
    const string& consulta
) {
    RankingPorRelevancia ranking;
    vector<pair<int, Pelicula>> conteo;

    for (const auto& p : resultados) {
        int score = ranking.calcularScore(p, consulta);
        conteo.push_back({score, p});
    }

    sort(conteo.begin(), conteo.end(),
         [](const pair<int, Pelicula>& a, const pair<int, Pelicula>& b) {
             return a.first > b.first;
         });

    vector<Pelicula> resultado;
    int limite = min(5, static_cast<int>(conteo.size()));

    for (int i = 0; i < limite; i++) {
        resultado.push_back(conteo[i].second);
    }

    return resultado;
}


// ── Indexación del catálogo en el árbol de sufijos ────────────────────────────
inline void indexarCatalogo(
    const unordered_map<int, Pelicula>& catalogo,
    SuffixTree<int>& arbol
) {
    for (const auto& par : catalogo) {
        const Pelicula& p = par.second;

        // Campos de alta prioridad: búsqueda parcial por sufijos activa
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
inline vector<Pelicula> buscarConSuffixTree(
    const SuffixTree<int>& arbol,
    const unordered_map<int, Pelicula>& catalogo,
    const string& consulta
) {
    vector<Pelicula> resultados;
    vector<string> tokensConsulta = tokenizar(consulta);

    if (tokensConsulta.empty()) {
        return resultados;
    }

    vector<int> todosLosIds;

    for (const auto& token : tokensConsulta) {
        vector<int> idsEncontrados = arbol.buscar(token);
        todosLosIds.insert(todosLosIds.end(), idsEncontrados.begin(), idsEncontrados.end());
    }

    sort(todosLosIds.begin(), todosLosIds.end());

    auto ultimoIdUnico = unique(todosLosIds.begin(), todosLosIds.end());
    todosLosIds.erase(ultimoIdUnico, todosLosIds.end());

    for (int id : todosLosIds) {
        auto it = catalogo.find(id);

        if (it != catalogo.end()) {
            resultados.push_back(it->second);
        }
    }

    return resultados;
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
        return buscarConSuffixTree(arbol, db, consulta);
    }
};


class BuscadorPorTag : public Buscador {
public:
    vector<Pelicula> buscar(
        const unordered_map<int, Pelicula>& db,
        const SuffixTree<int>&,
        const string& consulta
    ) const override {
        return buscarPorTag(db, consulta);
    }
};


class BuscadorFactory {
public:
    static unique_ptr<Buscador> crearBuscador(bool esBusquedaPorTag) {
        if (esBusquedaPorTag) {
            return make_unique<BuscadorPorTag>();
        }

        return make_unique<BuscadorPorTexto>();
    }
};
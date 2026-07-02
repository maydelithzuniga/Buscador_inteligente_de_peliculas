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

static constexpr int COL_ANIO      = 0;
static constexpr int COL_TITULO    = 1;
static constexpr int COL_DIRECTOR  = 3;
static constexpr int COL_CAST      = 4;
static constexpr int COL_GENERO    = 5;
static constexpr int COL_SINOPSIS  = 7;


struct Pelicula {
    int id;
    string anio;
    string titulo;
    string sinopsis;
    vector<string> directores;
    vector<string> actores;
    vector<string> generos;
};


inline vector<string> parsearLinea(const string& linea) {
    vector<string> fila;
    string celda;
    bool en_comillas = false;

    for (size_t i = 0; i < linea.size(); i++) {
        char c = linea[i];

        if (c == '"') {
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



inline vector<int> idsCandidatosPorSuffixTree(
    const SuffixTree<int>& arbol,
    const vector<string>& tokensConsulta
);


enum class TipoTag {
    DIRECTOR,
    ACTOR,
    GENERO,
    ANIO
};


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

        for (const auto& t : tokenizar(p.titulo)) {
            for (const auto& token : tokens) {
                if (t.find(token) != string::npos) {
                    score += 100;
                }
            }
        }

        for (const auto& genero : p.generos) {
            for (const auto& palabra : tokenizar(genero)) {
                for (const auto& token : tokens) {
                    if (palabra.find(token) != string::npos) {
                        score += 50;
                    }
                }
            }
        }

        for (const auto& d : p.directores) {
            for (const auto& t : tokenizar(d)) {
                for (const auto& token : tokens) {
                    if (t.find(token) != string::npos) {
                        score += 10;
                    }
                }
            }
        }

        for (const auto& a : p.actores) {
            for (const auto& t : tokenizar(a)) {
                for (const auto& token : tokens) {
                    if (t.find(token) != string::npos) {
                        score += 10;
                    }
                }
            }
        }

        for (const auto& t : tokenizar(p.anio)) {
            for (const auto& token : tokens) {
                if (t == token) {
                    score += 30;
                }
            }
        }

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


class RankingPorAnio : public RankingStrategy {
public:
    int calcularScore(const Pelicula& p, const string& consulta) const override {
        int score = 0;

        int anioPelicula = 0;
        try {
            anioPelicula = stoi(p.anio);
        } catch (...) {
            return 0; 
        }

        for (const auto& token : tokenizar(consulta)) {
            try {
                int anioBuscado = stoi(token);

                if (anioPelicula == anioBuscado) {
                    score += 1000; 
                } else {
                    
                    int distancia = abs(anioPelicula - anioBuscado);
                    score += max(0, 200 - distancia);
                }
            } catch (...) {
                
            }
        }

        score += anioPelicula / 100;

        return score;
    }
};


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

inline vector<Pelicula> ordenarPorRelevancia(
    const vector<Pelicula>& resultados,
    const string& consulta
) {
    RankingPorRelevancia estrategia;
    return ordenarConEstrategia(resultados, consulta, estrategia);
}

inline vector<Pelicula> top5(
    const vector<Pelicula>& resultados,
    const string& consulta
) {
    vector<Pelicula> ordenados = ordenarPorRelevancia(resultados, consulta);
    int limite = min(5, static_cast<int>(ordenados.size()));
    return vector<Pelicula>(ordenados.begin(), ordenados.begin() + limite);
}


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
    
    vector<int> idsBase = likesSesion;

    if (idsBase.empty()) {
        idsBase = cargarLikesDesdeArchivo();
    }

    if (idsBase.empty()) {
        
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
        
        return peliculasAleatorias(db, 5);
    }

    auto esBase = [&idsBase](int id) {
        return find(idsBase.begin(), idsBase.end(), id) != idsBase.end();
    };

    
    vector<pair<int, Pelicula>> candidatosGenero; 

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



inline void indexarCatalogo(
    const unordered_map<int, Pelicula>& catalogo,
    SuffixTree<int>& arbol
) {
    for (const auto& par : catalogo) {
        const Pelicula& p = par.second;

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

        for (const auto& t : tokenizar(p.sinopsis)) {
            arbol.insertarPalabraCompleta(t, p.id);
        }
    }
}


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



inline vector<Pelicula> buscarPorTextoConPrioridad(
    const unordered_map<int, Pelicula>& catalogo,
    const SuffixTree<int>& arbol,
    const string& consultaOriginal
) {
    vector<string> tokensConsulta = tokenizar(consultaOriginal);

    if (tokensConsulta.empty()) {
        return {};
    }

    vector<int> idsCandidatos = idsCandidatosPorSuffixTree(arbol, tokensConsulta);

    vector<Pelicula> candidatos;
    candidatos.reserve(idsCandidatos.size());
    for (int id : idsCandidatos) {
        auto it = catalogo.find(id);
        if (it != catalogo.end()) candidatos.push_back(it->second);
    }

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

    vector<Pelicula> porSustring = buscarSustringEnSinopsisParalelo(catalogo, tokensConsulta);

    static std::mt19937 generadorAleatorio2(std::random_device{}());
    std::shuffle(porSustring.begin(), porSustring.end(), generadorAleatorio2);

    return porSustring;
}



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
        vector<Pelicula> resultados = buscarPorTag(db, arbol, consulta, tipo);

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
    static unique_ptr<Buscador> crearBuscador(bool esBusquedaPorTag, TipoTag tipo = TipoTag::GENERO) {
        if (esBusquedaPorTag) {
            return make_unique<BuscadorPorTag>(tipo);
        }

        return make_unique<BuscadorPorTexto>();
    }
};

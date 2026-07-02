#include "MotorBusqueda.h"
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <sstream>
#include <iomanip>

using namespace std;

// Función auxiliar para escapar strings en JSON
string escapeJson(const string& s) {
    ostringstream o;
    for (char c : s) {
        if (c == '"') o << "\\\"";
        else if (c == '\\') o << "\\\\";
        else if (c == '\b') o << "\\b";
        else if (c == '\f') o << "\\f";
        else if (c == '\n') o << "\\n";
        else if (c == '\r') o << "\\r";
        else if (c == '\t') o << "\\t";
        else if (0 <= c && c <= 0x1f) {
            o << "\\u" << hex << setw(4) << setfill('0') << (int)c;
        }
        else o << c;
    }
    return o.str();
}

string arrayToJson(const vector<string>& vec) {
    string json = "[";
    for (size_t i = 0; i < vec.size(); ++i) {
        json += "\"" + escapeJson(vec[i]) + "\"";
        if (i < vec.size() - 1) json += ", ";
    }
    json += "]";
    return json;
}

string peliculaToJson(const Pelicula& p) {
    ostringstream o;
    o << "{"
      << "\"id\": " << p.id << ", "
      << "\"anio\": \"" << escapeJson(p.anio) << "\", "
      << "\"titulo\": \"" << escapeJson(p.titulo) << "\", "
      << "\"sinopsis\": \"" << escapeJson(p.sinopsis) << "\", "
      << "\"directores\": " << arrayToJson(p.directores) << ", "
      << "\"actores\": " << arrayToJson(p.actores) << ", "
      << "\"generos\": " << arrayToJson(p.generos)
      << "}";
    return o.str();
}

int main() {
    // 1. Cargar la base de datos
    // Hacemos que cout y cerr se vacíen en un lugar que el frontend no interprete
    // o al menos marcamos claramente cuando estamos listos.
    cerr << "[API] Cargando base de datos..." << endl;
    unordered_map<int, Pelicula> db = cargarCSV("wiki_movie_plots_deduped_final.csv");
    if (db.empty()) {
        cerr << "[API] No se pudo cargar la base de datos." << endl;
        return 1;
    }

    // 2. Indexar Arbol
    cerr << "[API] Indexando palabras clave en el Arbol de Sufijos..." << endl;
    SuffixTree<int> arbolBusqueda;
    indexarCatalogo(db, arbolBusqueda);
    
    // Señal de "Listo" para el servidor Node.js
    cout << "READY" << endl;
    cerr << "[API] Sistema listo y esperando comandos." << endl;

    string command;
    // Bucle para leer de stdin
    while (getline(cin, command)) {
        if (command == "EXIT") break;
        if (command.empty()) continue;

        // Comando esperado: SEARCH_TEXT <query> o SEARCH_TAG <query>
        bool esBusquedaPorTag = false;
        string consulta = "";

        if (command.find("SEARCH_TEXT ") == 0 || command.find("SEARCH_TAG ") == 0) {
            bool esBusquedaPorTag = (command.find("SEARCH_TAG ") == 0);
            string args = command.substr(12); // "SEARCH_TEXT " o "SEARCH_TAG " tienen la misma longitud si asumimos que TAG tiene un espacio extra? No, "SEARCH_TAG " es 11.
            if (esBusquedaPorTag) args = command.substr(11);
            
            stringstream ss(args);
            int offset, limit;
            ss >> offset >> limit;
            
            string consulta;
            getline(ss, consulta);
            size_t start = consulta.find_first_not_of(" \t");
            if (start != string::npos) consulta = consulta.substr(start);
            else consulta = "";

            vector<Pelicula> resultados;
            if (!consulta.empty()) {
                auto buscador = BuscadorFactory::crearBuscador(esBusquedaPorTag);
                resultados = buscador->buscar(db, arbolBusqueda, consulta);
                
                RankingPorRelevancia ranking;
                vector<pair<int, Pelicula>> conteo;
                for (const auto& p : resultados) {
                    conteo.push_back({ranking.calcularScore(p, consulta), p});
                }
                sort(conteo.begin(), conteo.end(), [](const pair<int, Pelicula>& a, const pair<int, Pelicula>& b) { return a.first > b.first; });
                
                int totalCoincidencias = conteo.size();
                resultados.clear();
                
                int startIndex = min(offset, totalCoincidencias);
                int endIndex = min(offset + limit, totalCoincidencias);
                
                for (int i = startIndex; i < endIndex; i++) {
                    resultados.push_back(conteo[i].second);
                }

                // Convertir y emitir JSON array con el total
                cout << "{ \"totalCoincidencias\": " << totalCoincidencias << ", \"peliculas\": [";
                for (size_t i = 0; i < resultados.size(); ++i) {
                    cout << peliculaToJson(resultados[i]);
                    if (i < resultados.size() - 1) cout << ", ";
                }
                cout << "] }" << endl;
            } else {
                cout << "{ \"totalCoincidencias\": 0, \"peliculas\": [] }" << endl;
            }
        } else {
            cout << "{ \"totalCoincidencias\": 0, \"peliculas\": [] }" << endl;
        }

    }

    return 0;
}

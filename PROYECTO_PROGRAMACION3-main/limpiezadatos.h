
#pragma once
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <future>
#include <thread>
#include <algorithm>
#include <cctype>
using namespace std;

class limpiezadatos {
    string archivo_entrada="wiki_movie_plots_deduped.csv";
    string archivo_final="wiki_movie_plots_deduped_final.csv";

    static constexpr int NUM_CAMPOS_ESTRUCTURADOS = 7;

public:
    limpiezadatos()=default;
    ~limpiezadatos()=default;

    string trim(const string& s) {
        size_t inicio = s.find_first_not_of(" \t\r\n");
        if (inicio == string::npos) return ""; 
        size_t fin = s.find_last_not_of(" \t\r\n");
        return s.substr(inicio, fin - inicio + 1);
    }

    string limpiar_espacio(const string& s) {
        string resultado = s;
        size_t pos = 0;
        while ((pos = resultado.find("\r\n", pos)) != string::npos) {
            resultado.replace(pos, 2, " ");
        }
        pos = 0;
        while ((pos = resultado.find("\r", pos)) != string::npos) {
            resultado.replace(pos, 1, " ");
        }
        pos = 0;
        while ((pos = resultado.find('\n', pos)) != string::npos) {
            resultado.replace(pos, 1, " ");
        }
        return resultado;
    }

    static bool esInicioNuevaFila(const string& linea) {
        if (linea.size() < 5) return false;
        return isdigit((unsigned char)linea[0]) &&
               isdigit((unsigned char)linea[1]) &&
               isdigit((unsigned char)linea[2]) &&
               isdigit((unsigned char)linea[3]) &&
               linea[4] == ',';
    }


    string prepararSinopsis(string plot) {
        plot = trim(plot);

        
        if (plot.size() >= 2 && plot.front() == '"' && plot.back() == '"') {
            plot = plot.substr(1, plot.size() - 2);
        }

        plot = limpiar_espacio(plot);

        if (plot.empty() || plot == "Unknown") {
            plot = "unknown";
            return plot;
        }

       
        string escapado;
        escapado.reserve(plot.size());
        for (char c : plot) {
            if (c == '"') escapado += "\"\"";
            else escapado += c;
        }

        return "\"" + escapado + "\"";
    }

    inline string procesarFilaCadena(const string& linea) {
        
        vector<string> fila_procesada;
        string celda;
        bool en_comillas = false;
        size_t i = 0;

        while (i < linea.size() && (int)fila_procesada.size() < NUM_CAMPOS_ESTRUCTURADOS) {
            char c = linea[i];

            if (c == '"') {
                en_comillas = !en_comillas;
                celda += c;
            }
            else if (c == ',' && !en_comillas) {
                celda = trim(celda);
                if (celda == "" || celda == "Unknown" || celda == "\"\"" || celda == "\"Unknown\"") {
                    celda = "unknown";
                }
                celda = limpiar_espacio(celda);
                fila_procesada.push_back(celda);
                celda.clear();
            }
            else {
                celda += c;
            }

            i++;
        }

        if ((int)fila_procesada.size() < NUM_CAMPOS_ESTRUCTURADOS) {
            celda = trim(celda);
            if (celda == "" || celda == "Unknown" || celda == "\"\"" || celda == "\"Unknown\"") {
                celda = "unknown";
            }
            celda = limpiar_espacio(celda);
            fila_procesada.push_back(celda);
        }

        while ((int)fila_procesada.size() < NUM_CAMPOS_ESTRUCTURADOS) {
            fila_procesada.push_back("\"\"");
        }

        string sinopsisCruda = (i < linea.size()) ? linea.substr(i) : "";
        string sinopsisLista = prepararSinopsis(sinopsisCruda);
        fila_procesada.push_back(sinopsisLista.empty() ? "\"\"" : sinopsisLista);

        string resultado;
        for (size_t k = 0; k < fila_procesada.size(); k++) {
            resultado += fila_procesada[k];
            if (k != fila_procesada.size() - 1) {
                resultado += ",";
            }
        }
        resultado += "\n";
        return resultado;
    }

    void limpiardatoscsv() {
        auto inic = high_resolution_clock::now();

        ifstream entrada(archivo_entrada);
        if (!entrada.is_open()) {
            cout << "Error al abrir archivo de entrada" << endl;
            return;
        }

        string linea;
        vector<string> lineas_crudas;

        while (getline(entrada, linea)) {
            if (!linea.empty() && linea.back() == '\r')
                linea.pop_back();

            if (lineas_crudas.empty() || esInicioNuevaFila(linea)) {
                lineas_crudas.push_back(linea);
            } else {
                lineas_crudas.back() += " " + linea;
            }
        }

        entrada.close();

        unsigned int numHilos = thread::hardware_concurrency();
        if (numHilos == 0) numHilos = 4; 

        size_t totalLineas = lineas_crudas.size();
        size_t tamanoChunk = totalLineas / numHilos;

        vector<string> lineas_limpias(totalLineas);
        vector<future<void>> futuros;

        for (unsigned int i = 0; i < numHilos; ++i) {
            size_t inicio = i * tamanoChunk;
            size_t fin = (i == numHilos - 1) ? totalLineas : inicio + tamanoChunk;

            futuros.push_back(async(launch::async, [this, inicio, fin, &lineas_crudas, &lineas_limpias]() {
                for (size_t j = inicio; j < fin; ++j) {
                    lineas_limpias[j] = this->procesarFilaCadena(lineas_crudas[j]);
                }
            }));
        }

        for (auto& f : futuros) {
            f.get();
        }

        ofstream salida(archivo_final);
        if (!salida.is_open()) {
            cout << "Error al abrir archivo de salida" << endl;
            return;
        }

        for (const auto& linea_lista : lineas_limpias) {
            salida << linea_lista;
        }
        salida.close();

        cout << "Archivo _copy (final) generado correctamente EN PARALELO." << endl;

        auto fin = high_resolution_clock::now();

        auto duracion = duration_cast<milliseconds>(fin - inic);

        cout << "Tiempo: " << duracion.count() << " ms" << endl;

    }
};

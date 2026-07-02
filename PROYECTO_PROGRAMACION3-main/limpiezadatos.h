// Created by maydelithzuniga on 07/05/2026.
//

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

    // Numero de columnas "estructuradas" antes de la Sinopsis/Plot:
    // Anio, Titulo, Origen, Director, Cast, Genero, WikiPage = 7.
    // La 8va (Plot) es todo lo que sobra despues de la 7ma coma.
    static constexpr int NUM_CAMPOS_ESTRUCTURADOS = 7;

public:
    limpiezadatos()=default;
    ~limpiezadatos()=default;

    // función trim
    string trim(const string& s) {
        size_t inicio = s.find_first_not_of(" \t\r\n");
        if (inicio == string::npos) return ""; // solo espacios → vacío
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

    // Igual criterio que MotorBusqueda::esInicioNuevaPelicula: una fila nueva
    // del CSV original siempre arranca con el Anio (4 digitos) seguido de
    // coma. Es mucho mas confiable que contar comillas, porque no se
    // confunde cuando la Sinopsis trae comillas sueltas mal escapadas.
    static bool esInicioNuevaFila(const string& linea) {
        if (linea.size() < 5) return false;
        return isdigit((unsigned char)linea[0]) &&
               isdigit((unsigned char)linea[1]) &&
               isdigit((unsigned char)linea[2]) &&
               isdigit((unsigned char)linea[3]) &&
               linea[4] == ',';
    }

    // ── Limpia y escapa el contenido de la Sinopsis/Plot para que el CSV de
    // salida quede SIEMPRE valido, sin importar cuan mal escapada venga en el
    // dataset original (comillas sueltas, dialogos con "" sin duplicar, etc).
    string prepararSinopsis(string plot) {
        plot = trim(plot);

        // Si el campo original vino entre comillas, se las quitamos: las
        // volvemos a poner nosotros mismos mas abajo, ya escapadas bien.
        if (plot.size() >= 2 && plot.front() == '"' && plot.back() == '"') {
            plot = plot.substr(1, plot.size() - 2);
        }

        plot = limpiar_espacio(plot);

        if (plot.empty() || plot == "Unknown") {
            plot = "unknown";
            return plot;
        }

        // Escapamos cualquier comilla interna duplicandola (regla estandar
        // de CSV). Esto es lo que garantiza que, sin importar que tan rota
        // venga la sinopsis original, el archivo de salida siempre se pueda
        // volver a leer correctamente y respete el orden de las columnas.
        string escapado;
        escapado.reserve(plot.size());
        for (char c : plot) {
            if (c == '"') escapado += "\"\"";
            else escapado += c;
        }

        return "\"" + escapado + "\"";
    }

    inline string procesarFilaCadena(const string& linea) {
        // Solo partimos las primeras NUM_CAMPOS_ESTRUCTURADOS columnas
        // (Anio, Titulo, Origen, Director, Cast, Genero, WikiPage) respetando
        // comillas. Estas columnas casi nunca traen comillas sueltas mal
        // escapadas, asi que partirlas por comas es seguro y mantiene el
        // orden correcto. La Sinopsis (8va columna) NO se vuelve a partir
        // por comas: es todo lo que sobra despues de la 7ma coma, tal cual,
        // sin depender de que sus comillas esten balanceadas.
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

        // Si la linea se acabo antes de juntar las 7 columnas esperadas
        // (fila incompleta/corrupta), guardamos lo que alcanzamos a leer.
        if ((int)fila_procesada.size() < NUM_CAMPOS_ESTRUCTURADOS) {
            celda = trim(celda);
            if (celda == "" || celda == "Unknown" || celda == "\"\"" || celda == "\"Unknown\"") {
                celda = "unknown";
            }
            celda = limpiar_espacio(celda);
            fila_procesada.push_back(celda);
        }

        // Si aun asi faltan columnas estructuradas (la fila original vino
        // con menos comas de las esperadas, es decir con columnas faltantes
        // de verdad), NO se descarta la fila: se completan las que faltan
        // con "" (vacio, formato CSV valido) para respetar la posicion de
        // las demas columnas y que la fila siempre tenga las 8 columnas al
        // leerla despues.
        while ((int)fila_procesada.size() < NUM_CAMPOS_ESTRUCTURADOS) {
            fila_procesada.push_back("\"\"");
        }

        // Todo lo que sobra desde 'i' hasta el final de la linea es la
        // Sinopsis completa (columna 8), tal cual, sin volver a partirla.
        // Si no quedo nada (la fila no tenia Sinopsis), tambien se completa
        // con "" en vez de dejar la columna faltante.
        string sinopsisCruda = (i < linea.size()) ? linea.substr(i) : "";
        string sinopsisLista = prepararSinopsis(sinopsisCruda);
        fila_procesada.push_back(sinopsisLista.empty() ? "\"\"" : sinopsisLista);

        // Reconstruimos la línea en formato CSV
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

        // ── FASE 1: Lectura secuencial, reconstruyendo registros multilinea ──────
        // La Sinopsis puede venir en varios parrafos con saltos de linea
        // reales (\n) dentro del campo entre comillas. getline() corta en
        // cada \n, asi que una sola pelicula puede llegar partida en varias
        // "lineas" crudas. Usamos el mismo criterio confiable que
        // MotorBusqueda::cargarCSV: una fila NUEVA siempre arranca con 4
        // digitos (el Anio) seguidos de coma. Si una linea no cumple eso,
        // es continuacion de la sinopsis de la fila anterior.
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

        // ── FASE 2: Procesamiento y Limpieza de caracteres en PARALELO ───────────
        unsigned int numHilos = thread::hardware_concurrency();
        if (numHilos == 0) numHilos = 4; // Resguardo si no se detectan núcleos

        size_t totalLineas = lineas_crudas.size();
        size_t tamanoChunk = totalLineas / numHilos;

        // Vector global donde se guardarán las líneas finales ya limpias
        vector<string> lineas_limpias(totalLineas);
        vector<future<void>> futuros;

        for (unsigned int i = 0; i < numHilos; ++i) {
            size_t inicio = i * tamanoChunk;
            size_t fin = (i == numHilos - 1) ? totalLineas : inicio + tamanoChunk;

            // Lanzamos hilos de trabajo asíncronos
            futuros.push_back(async(launch::async, [this, inicio, fin, &lineas_crudas, &lineas_limpias]() {
                for (size_t j = inicio; j < fin; ++j) {
                    // Cada hilo escribe directamente en el índice asignado sin colisiones
                    lineas_limpias[j] = this->procesarFilaCadena(lineas_crudas[j]);
                }
            }));
        }

        // Esperamos a que todos los hilos terminen su procesamiento de texto
        for (auto& f : futuros) {
            f.get();
        }

        // ── FASE 3: Escritura secuencial ordenada en el archivo final ────────────
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

        // Calcular duración en milisegundos
        auto duracion = duration_cast<milliseconds>(fin - inic);

        cout << "Tiempo: " << duracion.count() << " ms" << endl;

    }
};
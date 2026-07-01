#include "Vistas.h"
#include "MotorBusqueda.h"
#include "limpiezadatos.h"

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

int main() {
    // ── 1. Limpieza y generación del CSV final ────────────────────────────────
    limpiezadatos limpieza;
    limpieza.limpiardatoscsv();

    // ── 2. Carga de la base de datos ──────────────────────────────────────────
    EstadoPantalla estado = MENU_PRINCIPAL;
    std::unordered_map<int, Pelicula> db = cargarCSV();

    if (db.empty()) {
        std::cout << "No se pudo cargar la base de datos.\n";
        return 1;
    }

    // ── 3. Crear e indexar el Árbol de Sufijos ────────────────────────────────
    SuffixTree<int> arbolBusqueda;

    std::cout << "[Sistema] Indexando palabras clave en el Arbol de Sufijos... ";
    indexarCatalogo(db, arbolBusqueda);
    std::cout << "Listo!" << std::endl;

    // ── 4. Estado de la aplicación ────────────────────────────────────────────
    std::vector<Pelicula> resultados;
    std::vector<int> likes;
    std::vector<int> verMasTarde;
    std::string ultimaBusqueda;

    // ── 5. Bucle principal ────────────────────────────────────────────────────
    while (estado != SALIR) {
        switch (estado) {
            case MENU_PRINCIPAL:
                estado = vistaMenuPrincipal();
                break;

            case BUSCAR: {
                std::string consulta;
                bool esBusquedaPorTag = false;

                estado = vistaBuscar(consulta, esBusquedaPorTag);

                if (!consulta.empty()) {
                    // ── PATRÓN FACTORY METHOD ────────────────────────────────
                    // La fábrica decide qué tipo de buscador crear:
                    // - BuscadorPorTexto  -> usa el SuffixTree
                    // - BuscadorPorTag    -> busca por director, actor o género
                    auto buscador = BuscadorFactory::crearBuscador(esBusquedaPorTag);

                    resultados = buscador->buscar(db, arbolBusqueda, consulta);

                    // ── PATRÓN STRATEGY ──────────────────────────────────────
                    // top5 usa RankingPorRelevancia, que implementa RankingStrategy.
                    resultados = top5(resultados, consulta);

                    ultimaBusqueda = consulta;
                }

                break;
            }

            case RESULTADOS:
                estado = vistaResultados(resultados, likes, verMasTarde);
                break;

            case VER_MAS_TARDE:
                estado = vistaVerMasTarde(verMasTarde, db, likes);
                break;

            case PELICULAS_LIKE:
                estado = vistaPeliculasLike(likes, db);
                break;

            case PELICULAS_RECOMENDADA:
                estado = vistaPeliculasRecomendada(db, likes);
                break;

            default:
                estado = SALIR;
                break;
        }
    }

    limpiarPantalla();
    std::cout << "Hasta luego!\n";

    return 0;
}

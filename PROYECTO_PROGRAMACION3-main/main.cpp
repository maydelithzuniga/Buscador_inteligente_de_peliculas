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

    // Lista de recomendadas: se calcula UNA sola vez cada vez que se entra al
    // menu principal, y esa MISMA lista es la que se muestra tanto en
    // "Peliculas que te pueden interesar" (dentro del menu) como en
    // "Ver pelicula recomendada". Asi ambas pantallas siempre coinciden.
    std::vector<Pelicula> recomendadasActuales;

    // ── 5. Bucle principal ────────────────────────────────────────────────────
    while (estado != SALIR) {
        switch (estado) {
            case MENU_PRINCIPAL:
                recomendadasActuales = obtenerPeliculasRecomendadas(db, likes);
                estado = vistaMenuPrincipal(db, verMasTarde, recomendadasActuales);
                break;

            case BUSCAR: {
                std::string consulta;
                bool esBusquedaPorTag = false;
                TipoTag tipoTag = TipoTag::GENERO; // valor por defecto, solo aplica si esBusquedaPorTag == true

                estado = vistaBuscar(consulta, esBusquedaPorTag, tipoTag);

                if (!consulta.empty()) {
                    // ── PATRÓN FACTORY METHOD ────────────────────────────────
                    // La fábrica decide qué tipo de buscador crear:
                    // - BuscadorPorTexto  -> prioriza titulo (empieza-con /
                    //   contiene) y solo cae a sinopsis (orden aleatorio) si
                    //   ningun titulo coincide. Ver buscarPorTextoConPrioridad.
                    // - BuscadorPorTag    -> busca por director, actor, genero
                    //   o anio (el campo especifico viene dado por tipoTag) y
                    //   usa el ranking por relevancia (Strategy).
                    // Cada Buscador ya devuelve sus resultados en el orden
                    // final: la vista solo pagina con ResultadosPaginador
                    // (patron Iterator), no vuelve a reordenar.
                    auto buscador = BuscadorFactory::crearBuscador(esBusquedaPorTag, tipoTag);

                    resultados = buscador->buscar(db, arbolBusqueda, consulta);

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
                estado = vistaPeliculasRecomendada(recomendadasActuales, likes, verMasTarde);
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
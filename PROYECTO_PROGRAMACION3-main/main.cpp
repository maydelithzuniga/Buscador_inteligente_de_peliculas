#include "Vistas.h"
#include "MotorBusqueda.h"
#include "limpiezadatos.h"

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

int main() {
    limpiezadatos limpieza;
    limpieza.limpiardatoscsv();

    EstadoPantalla estado = MENU_PRINCIPAL;
    std::unordered_map<int, Pelicula> db = cargarCSV();

    if (db.empty()) {
        std::cout << "No se pudo cargar la base de datos.\n";
        return 1;
    }

    SuffixTree<int> arbolBusqueda;

    std::cout << "[Sistema] Indexando palabras clave en el Arbol de Sufijos... ";
    indexarCatalogo(db, arbolBusqueda);
    std::cout << "Listo!" << std::endl;

    std::vector<Pelicula> resultados;
    std::vector<int> likes;
    std::vector<int> verMasTarde;
    std::string ultimaBusqueda;

    std::vector<Pelicula> recomendadasActuales;

    while (estado != SALIR) {
        switch (estado) {
            case MENU_PRINCIPAL:
                recomendadasActuales = obtenerPeliculasRecomendadas(db, likes);
                estado = vistaMenuPrincipal(db, verMasTarde, recomendadasActuales);
                break;

            case BUSCAR: {
                std::string consulta;
                bool esBusquedaPorTag = false;
                TipoTag tipoTag = TipoTag::GENERO; 

                estado = vistaBuscar(consulta, esBusquedaPorTag, tipoTag);

                if (!consulta.empty()) {
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

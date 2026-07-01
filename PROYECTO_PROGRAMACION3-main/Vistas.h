#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "ConsoleUtils.h"
#include "MotorBusqueda.h"

enum EstadoPantalla {
    MENU_PRINCIPAL,
    BUSCAR,
    RESULTADOS,
    VER_MAS_TARDE,
    SALIR,
    PELICULAS_LIKE,
    PELICULAS_RECOMENDADA
};

// ── Menu Principal ────────────────────────────────────────────────────────────
inline EstadoPantalla vistaMenuPrincipal() {
    limpiarPantalla();
    std::cout << "========================================\n";
    std::cout << "                NETFLIX                 \n";
    std::cout << "========================================\n";
    std::cout << "  [1] Buscar pelicula\n";
    std::cout << "  [2] Ver mas tarde\n";
    std::cout << "  [3] Peliculas que le di like\n";
    std::cout << "  [4] Ver pelicula recomendada\n";
    std::cout << "  [0] Salir\n";
    std::cout << "----------------------------------------\n";
    std::cout << "  Peliculas que te pueden interesar\n";
    std::cout << "  -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.\n";
    std::cout << "  1. Abre los ojos\n";
    std::cout << "  2. El ladron de orquideas\n";
    std::cout << "  3. The Empty Man\n";
    std::cout << "  4. La pantera rosa\n";
    std::cout << "  5. The Greasy Strangler\n";
    std::cout << "----------------------------------------\n";

    switch (leerOpcion(0, 4)) {
        case 1:  return BUSCAR;
        case 2:  return VER_MAS_TARDE;
        case 3:  return PELICULAS_LIKE;
        case 4:  return PELICULAS_RECOMENDADA;
        default: return SALIR;
    }
}

// ── Buscar — rellena 'consulta' y avisa si fue busqueda por tag ────────────────
inline EstadoPantalla vistaBuscar(std::string& consulta, bool& buscarPorTag) {
    limpiarPantalla();
    std::cout << "========================================\n";
    std::cout << "             BUSCAR                     \n";
    std::cout << "========================================\n";
    std::cout << "  [1] Por palabra clave\n";
    std::cout << "  [2] Por tag: director, casting o genero\n";
    std::cout << "  [0] Volver\n";
    std::cout << "----------------------------------------\n";

    int op = leerOpcion(0, 2);

    if (op == 0) {
        return MENU_PRINCIPAL;
    }

    buscarPorTag = (op == 2);

    if (buscarPorTag) {
        std::cout << "Ingrese director, actor o genero: ";
    } else {
        std::cout << "Ingrese palabra clave: ";
    }

    consulta = leerTexto();

    std::cout << "\nBuscando \"" << consulta << "\"...\n";
    pausar();

    return RESULTADOS;
}

// ── Helper: verifica si un ID ya esta en una lista ─────────────────────────────
inline bool contieneId(const std::vector<int>& lista, int id) {
    return std::find(lista.begin(), lista.end(), id) != lista.end();
}

// ─────────────────────────────────────────────────────────────────────────────
// PATRÓN COMMAND
// Cada acción del usuario se encapsula como un objeto comando.
// Esto evita que la vista modifique directamente la lógica de Like/Ver más tarde.
// ─────────────────────────────────────────────────────────────────────────────

class ComandoUsuario {
public:
    virtual void ejecutar() = 0;
    virtual ~ComandoUsuario() = default;
};

class ToggleLikeCommand : public ComandoUsuario {
private:
    std::vector<int>& likes;
    int idPelicula;

public:
    ToggleLikeCommand(std::vector<int>& likes, int idPelicula)
        : likes(likes), idPelicula(idPelicula) {}

    void ejecutar() override {
        if (contieneId(likes, idPelicula)) {
            likes.erase(
                std::remove(likes.begin(), likes.end(), idPelicula),
                likes.end()
            );
        } else {
            likes.push_back(idPelicula);
        }
    }
};

class ToggleVerMasTardeCommand : public ComandoUsuario {
private:
    std::vector<int>& verMasTarde;
    int idPelicula;

public:
    ToggleVerMasTardeCommand(std::vector<int>& verMasTarde, int idPelicula)
        : verMasTarde(verMasTarde), idPelicula(idPelicula) {}

    void ejecutar() override {
        if (contieneId(verMasTarde, idPelicula)) {
            verMasTarde.erase(
                std::remove(verMasTarde.begin(), verMasTarde.end(), idPelicula),
                verMasTarde.end()
            );
        } else {
            verMasTarde.push_back(idPelicula);
        }
    }
};

// ── Detalle de pelicula: sinopsis + Like + Ver mas tarde ──────────────────────
inline void mostrarDetallePelicula(
    const Pelicula& p,
    std::vector<int>& likes,
    std::vector<int>& verMasTarde
) {
    limpiarPantalla();
    std::cout << "========================================\n";
    std::cout << "          DETALLE DE PELICULA           \n";
    std::cout << "========================================\n";
    std::cout << "Titulo  : " << p.titulo << "\n";
    std::cout << "Anio    : " << p.anio   << "\n";

    std::cout << "Genero(s): ";
    for (const auto& g : p.generos) {
        std::cout << g << " ";
    }
    std::cout << "\n";

    std::cout << "Director(es): ";
    for (const auto& d : p.directores) {
        std::cout << d << " ";
    }
    std::cout << "\n";

    std::cout << "Actor(es): ";
    for (const auto& d : p.actores) {
        std::cout << d << ", ";
    }
    std::cout << "\n";

    std::cout << "----------------------------------------\n";
    std::cout << "Sinopsis:\n" << p.sinopsis << "\n";
    std::cout << "----------------------------------------\n";

    while (true) {
        bool yaTieneLike   = contieneId(likes, p.id);
        bool yaVerMasTarde = contieneId(verMasTarde, p.id);

        std::cout << "  [1] "
                  << (yaTieneLike ? "Quitar like" : "Dar like")
                  << "\n";

        std::cout << "  [2] "
                  << (yaVerMasTarde ? "Quitar de Ver mas tarde" : "Agregar a Ver mas tarde")
                  << "\n";

        std::cout << "  [0] Volver\n";
        std::cout << "----------------------------------------\n";

        int op = leerOpcion(0, 2);

        if (op == 0) {
            return;
        }

        if (op == 1) {
            ToggleLikeCommand comando(likes, p.id);
            comando.ejecutar();

            std::cout << (yaTieneLike ? "  Like eliminado.\n" : "  Like agregado.\n");
        }

        if (op == 2) {
            ToggleVerMasTardeCommand comando(verMasTarde, p.id);
            comando.ejecutar();

            std::cout << (yaVerMasTarde ? "  Eliminado de Ver mas tarde.\n" : "  Agregado a Ver mas tarde.\n");
        }
    }
}

// ── Resultados: muestra 5 por pagina y permite ver detalle ────────────────────
inline EstadoPantalla vistaResultados(
    const std::vector<Pelicula>& resultados,
    std::vector<int>& likes,
    std::vector<int>& verMasTarde
) {
    int pagina = 0;
    const int tamPagina = 5;

    limpiarPantalla();

    while (true) {
        std::cout << "========================================\n";
        std::cout << "            RESULTADOS                  \n";
        std::cout << "========================================\n";

        if (resultados.empty()) {
            std::cout << "  No se encontraron peliculas.\n";
            std::cout << "----------------------------------------\n";
            std::cout << "  [0] Volver al menu principal\n";
            std::cout << "----------------------------------------\n";

            leerOpcion(0, 0);
            return MENU_PRINCIPAL;
        }

        int inicio = pagina * tamPagina;
        int fin = std::min(inicio + tamPagina, static_cast<int>(resultados.size()));

        for (int i = inicio; i < fin; i++) {
            std::cout << "  [" << (i - inicio + 1) << "] "
                      << resultados[i].titulo << "\n";
        }

        std::cout << "----------------------------------------\n";
        std::cout << "Pagina " << (pagina + 1) << "\n";
        std::cout << "  [1-" << (fin - inicio) << "] Ver detalle\n";

        if (fin < static_cast<int>(resultados.size())) {
            std::cout << "  [6] Siguientes 5\n";
        }

        if (pagina > 0) {
            std::cout << "  [7] Anteriores 5\n";
        }

        std::cout << "  [0] Volver al menu principal\n";
        std::cout << "----------------------------------------\n";
        std::cout.flush();

        int op = leerOpcion(0, 7);

        if (op == 0) {
            return MENU_PRINCIPAL;
        }

        if (op >= 1 && op <= fin - inicio) {
            mostrarDetallePelicula(resultados[inicio + op - 1], likes, verMasTarde);
        } else if (op == 6 && fin < static_cast<int>(resultados.size())) {
            pagina++;
        } else if (op == 7 && pagina > 0) {
            pagina--;
        }
    }
}

// ── Ver Mas Tarde ─────────────────────────────────────────────────────────────
inline EstadoPantalla vistaVerMasTarde(
    const std::vector<int>& verMasTarde,
    const std::unordered_map<int, Pelicula>& db,
    std::vector<int>& likes
) {
    limpiarPantalla();

    std::cout << "========================================\n";
    std::cout << "           VER MAS TARDE                \n";
    std::cout << "========================================\n";

    if (verMasTarde.empty()) {
        std::cout << "  (Lista vacia)\n";
    } else {
        int i = 1;

        for (int id : verMasTarde) {
            auto it = db.find(id);

            if (it != db.end()) {
                std::cout << "  [" << i++ << "] "
                          << it->second.titulo
                          << " (" << it->second.anio << ")\n";
            }
        }
    }

    std::cout << "----------------------------------------\n";
    std::cout << "  [0] Volver al menu principal\n";
    std::cout << "----------------------------------------\n";

    leerOpcion(0, 0);
    return MENU_PRINCIPAL;
}

// ── Peliculas con Like ────────────────────────────────────────────────────────
inline EstadoPantalla vistaPeliculasLike(
    const std::vector<int>& likes,
    const std::unordered_map<int, Pelicula>& db
) {
    limpiarPantalla();

    std::cout << "========================================\n";
    std::cout << "        PELICULAS QUE LE DI LIKE        \n";
    std::cout << "========================================\n";

    if (likes.empty()) {
        std::cout << "  Todavia no le diste like a ninguna pelicula.\n";
    } else {
        int i = 1;

        for (int id : likes) {
            auto it = db.find(id);

            if (it != db.end()) {
                std::cout << "  [" << i++ << "] "
                          << it->second.titulo
                          << " (" << it->second.anio << ")\n";
            }
        }
    }

    std::cout << "----------------------------------------\n";
    std::cout << "  [0] Volver al menu principal\n";
    std::cout << "----------------------------------------\n";

    leerOpcion(0, 0);
    return MENU_PRINCIPAL;
}

// ── Ver pelicula recomendada ─────────────────────────────────────────────────
inline EstadoPantalla vistaPeliculasRecomendada(
    const std::unordered_map<int, Pelicula>& db,
    const std::vector<int>& likes
) {
    limpiarPantalla();

    std::cout << "========================================\n";
    std::cout << "        PELICULA RECOMENDADA            \n";
    std::cout << "========================================\n";

    if (likes.empty()) {
        std::cout << "  Todavia no hay recomendaciones.\n";
        std::cout << "  Dale like a alguna pelicula primero.\n";
    } else {
        int idLike = likes.back();
        auto itLike = db.find(idLike);

        if (itLike == db.end()) {
            std::cout << "  No se pudo generar recomendacion.\n";
        } else {
            const Pelicula& base = itLike->second;
            bool encontro = false;

            for (const auto& par : db) {
                const Pelicula& candidata = par.second;

                if (candidata.id == base.id) {
                    continue;
                }

                for (const auto& gBase : base.generos) {
                    for (const auto& gCand : candidata.generos) {
                        if (limpiar(gBase) == limpiar(gCand) && limpiar(gBase) != "unknown") {
                            std::cout << "  Basado en tu like a: " << base.titulo << "\n";
                            std::cout << "----------------------------------------\n";
                            std::cout << "  Te recomendamos:\n";
                            std::cout << "  " << candidata.titulo
                                      << " (" << candidata.anio << ")\n";
                            std::cout << "  Genero similar: " << gCand << "\n";

                            encontro = true;
                            break;
                        }
                    }

                    if (encontro) {
                        break;
                    }
                }

                if (encontro) {
                    break;
                }
            }

            if (!encontro) {
                std::cout << "  No se encontro una pelicula similar por genero.\n";
            }
        }
    }

    std::cout << "----------------------------------------\n";
    std::cout << "  [0] Volver al menu principal\n";
    std::cout << "----------------------------------------\n";

    leerOpcion(0, 0);
    return MENU_PRINCIPAL;
}
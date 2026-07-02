#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

#include "Consoleutils.h"
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
// Muestra:
//   - "Ver mas tarde": SOLO si el usuario ya agrego alguna pelicula a esa
//     lista. Si esta vacia, no se imprime ni el titulo de la seccion.
//   - "Peliculas que te pueden interesar": recibe la lista de recomendadas ya
//     calculada por main.cpp (obtenerPeliculasRecomendadas). Es DELIBERADO
//     que no se recalcule aca adentro: si se recalculara por separado cada
//     vez que se entra al menu Y cada vez que se entra a "Ver pelicula
//     recomendada", cuando no hay likes el algoritmo cae al azar y ambas
//     pantallas mostrarian listas distintas. Al pasar la misma lista ya
//     calculada, "que te pueden interesar" y "recomendadas" son siempre
//     exactamente las mismas peliculas.
inline EstadoPantalla vistaMenuPrincipal(
    const std::unordered_map<int, Pelicula>& db,
    const std::vector<int>& verMasTarde,
    const std::vector<Pelicula>& recomendadas
) {
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

    if (!verMasTarde.empty()) {
        std::cout << "  Ver mas tarde\n";
        std::cout << "  -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.\n";

        for (int id : verMasTarde) {
            auto it = db.find(id);
            if (it != db.end()) {
                std::cout << "  - " << it->second.titulo << "\n";
            }
        }

        std::cout << "----------------------------------------\n";
    }

    if (!recomendadas.empty()) {
        std::cout << "  Peliculas que te pueden interesar\n";
        std::cout << "  -.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.-.\n";

        int i = 1;
        for (const auto& p : recomendadas) {
            std::cout << "  " << i++ << ". " << p.titulo << "\n";
        }

        std::cout << "----------------------------------------\n";
    }

    switch (leerOpcion(0, 4)) {
        case 1:  return BUSCAR;
        case 2:  return VER_MAS_TARDE;
        case 3:  return PELICULAS_LIKE;
        case 4:  return PELICULAS_RECOMENDADA;
        default: return SALIR;
    }
}

// ── Buscar — rellena 'consulta', avisa si fue busqueda por tag y, de serlo,
// tambien indica en 'tipoTag' cual campo especifico eligio el usuario
// (Director, Actor, Genero o Anio). ────────────────────────────────────────────
inline EstadoPantalla vistaBuscar(std::string& consulta, bool& buscarPorTag, TipoTag& tipoTag) {
    limpiarPantalla();
    std::cout << "========================================\n";
    std::cout << "             BUSCAR                     \n";
    std::cout << "========================================\n";
    std::cout << "  [1] Por palabra clave\n";
    std::cout << "  [2] Por tag: director, actor, genero o anio\n";
    std::cout << "  [0] Volver\n";
    std::cout << "----------------------------------------\n";

    int op = leerOpcion(0, 2);

    if (op == 0) {
        return MENU_PRINCIPAL;
    }

    buscarPorTag = (op == 2);

    if (buscarPorTag) {
        std::cout << "----------------------------------------\n";
        std::cout << "  Buscar por tag:\n";
        std::cout << "  [1] Director\n";
        std::cout << "  [2] Actor\n";
        std::cout << "  [3] Genero\n";
        std::cout << "  [4] Anio\n";
        std::cout << "----------------------------------------\n";

        int opTag = leerOpcion(1, 4);

        switch (opTag) {
            case 1:
                tipoTag = TipoTag::DIRECTOR;
                std::cout << "Ingrese el nombre del director: ";
                break;
            case 2:
                tipoTag = TipoTag::ACTOR;
                std::cout << "Ingrese el nombre del actor: ";
                break;
            case 3:
                tipoTag = TipoTag::GENERO;
                std::cout << "Ingrese el genero: ";
                break;
            case 4:
                tipoTag = TipoTag::ANIO;
                std::cout << "Ingrese el anio: ";
                break;
        }
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

        // Persistimos el estado actual de likes en likes.txt, para que
        // sesiones futuras puedan usar este historial si el usuario todavia
        // no dio like a nada en esa nueva sesion (ver obtenerPeliculasRecomendadas).
        guardarLikesEnArchivo(likes);
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

// ─────────────────────────────────────────────────────────────────────────────
// PATRÓN ITERATOR
// Un iterador real (forward iterator): expone operator*, operator++ y
// operator!=, sin exponer el vector interno ni el indice absoluto. Esto es
// lo que permite recorrer la pagina actual con un range-for normal
// (for (const auto& p : paginador) ...) igual que cualquier contenedor de
// la STL, sin que el codigo cliente (la vista) sepa como esta almacenada
// la coleccion por dentro.
// ─────────────────────────────────────────────────────────────────────────────
class ResultadosIterator {
private:
    const std::vector<Pelicula>* resultados;
    size_t indice;

public:
    ResultadosIterator(const std::vector<Pelicula>* resultados, size_t indice)
        : resultados(resultados), indice(indice) {}

    const Pelicula& operator*() const { return (*resultados)[indice]; }
    const Pelicula* operator->() const { return &(*resultados)[indice]; }

    ResultadosIterator& operator++() { ++indice; return *this; }

    bool operator!=(const ResultadosIterator& otro) const { return indice != otro.indice; }
    bool operator==(const ResultadosIterator& otro) const { return indice == otro.indice; }
};

// Encapsula el recorrido pagina a pagina sobre TODOS los resultados de una
// busqueda. Antes 'top5' recortaba a 5 resultados y la vista nunca podia
// mostrar mas alla de esa pagina. Ahora MotorBusqueda::ordenarPorRelevancia /
// ordenarConEstrategia devuelven TODOS los resultados ordenados, y este
// paginador expone begin()/end() (Iterator real) para recorrer SOLO la
// pagina actual, de a 'tamPagina' en 'tamPagina', sin que la vista maneje
// indices absolutos a mano.
class ResultadosPaginador {
private:
    const std::vector<Pelicula>& resultados;
    int tamPagina;
    int paginaActual = 0;

public:
    ResultadosPaginador(const std::vector<Pelicula>& resultados, int tamPagina = 5)
        : resultados(resultados), tamPagina(tamPagina) {}

    bool vacio() const { return resultados.empty(); }

    int totalPaginas() const {
        if (resultados.empty()) return 0;
        return static_cast<int>((resultados.size() + tamPagina - 1) / tamPagina);
    }

    int numeroPaginaActual() const { return paginaActual + 1; }

    bool tieneSiguiente() const { return paginaActual + 1 < totalPaginas(); }
    bool tieneAnterior() const { return paginaActual > 0; }

    void siguiente() { if (tieneSiguiente()) paginaActual++; }
    void anterior() { if (tieneAnterior()) paginaActual--; }

    int inicioPagina() const { return paginaActual * tamPagina; }
    int finPagina() const {
        return std::min(inicioPagina() + tamPagina, static_cast<int>(resultados.size()));
    }

    int cantidadEnPaginaActual() const { return finPagina() - inicioPagina(); }

    const Pelicula& obtener(int indiceRelativo) const {
        return resultados[inicioPagina() + indiceRelativo];
    }

    // ── Interfaz de Iterator: recorre SOLO la pagina actual ───────────────────
    ResultadosIterator begin() const {
        return ResultadosIterator(&resultados, static_cast<size_t>(inicioPagina()));
    }

    ResultadosIterator end() const {
        return ResultadosIterator(&resultados, static_cast<size_t>(finPagina()));
    }
};

// ── Resultados: recorre TODOS los resultados con ResultadosPaginador ──────────
inline EstadoPantalla vistaResultados(
    const std::vector<Pelicula>& resultados,
    std::vector<int>& likes,
    std::vector<int>& verMasTarde
) {
    ResultadosPaginador paginador(resultados, 5);

    limpiarPantalla();

    while (true) {
        std::cout << "========================================\n";
        std::cout << "            RESULTADOS                  \n";
        std::cout << "========================================\n";

        if (paginador.vacio()) {
            std::cout << "  No se encontraron peliculas.\n";
            std::cout << "----------------------------------------\n";
            std::cout << "  [0] Volver al menu principal\n";
            std::cout << "----------------------------------------\n";

            leerOpcion(0, 0);
            return MENU_PRINCIPAL;
        }

        int cantidad = paginador.cantidadEnPaginaActual();

        // Recorrido con el Iterator real (begin()/end()) en vez de indices
        // manuales: paginador expone solo la pagina actual, sin que esta
        // vista sepa nada del vector completo de resultados.
        int numero = 1;
        for (const Pelicula& p : paginador) {
            std::cout << "  [" << numero++ << "] " << p.titulo << "\n";
        }

        std::cout << "----------------------------------------\n";
        std::cout << "Pagina " << paginador.numeroPaginaActual()
                   << " de " << paginador.totalPaginas()
                   << "  (" << resultados.size() << " resultados en total)\n";
        std::cout << "  [1-" << cantidad << "] Ver detalle\n";

        // El rango maximo de opciones valido depende de si realmente hay
        // pagina siguiente/anterior. Antes se pedia siempre "Opcion [0-7]"
        // aunque solo hubiera 1 resultado sin mas paginas, lo cual era
        // confuso/incorrecto. Ahora el limite superior se calcula segun lo
        // que efectivamente se imprime en pantalla.
        int maxOpcion = cantidad;

        if (paginador.tieneSiguiente()) {
            std::cout << "  [6] Siguientes 5\n";
            maxOpcion = std::max(maxOpcion, 6);
        }

        if (paginador.tieneAnterior()) {
            std::cout << "  [7] Anteriores 5\n";
            maxOpcion = std::max(maxOpcion, 7);
        }

        std::cout << "  [0] Volver al menu principal\n";
        std::cout << "----------------------------------------\n";
        std::cout.flush();

        int op = leerOpcion(0, maxOpcion);

        if (op == 0) {
            return MENU_PRINCIPAL;
        }

        if (op >= 1 && op <= cantidad) {
            mostrarDetallePelicula(paginador.obtener(op - 1), likes, verMasTarde);
        } else if (op == 6 && paginador.tieneSiguiente()) {
            paginador.siguiente();
        } else if (op == 7 && paginador.tieneAnterior()) {
            paginador.anterior();
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
// Recibe la lista de recomendadas YA CALCULADA por main.cpp (la misma que se
// mostro en "Peliculas que te pueden interesar" del menu principal, para que
// ambas pantallas coincidan siempre). Permite seleccionar una pelicula para
// ver su detalle completo (titulo, director, sinopsis, Like, Ver mas tarde),
// igual que en Resultados.
inline EstadoPantalla vistaPeliculasRecomendada(
    const std::vector<Pelicula>& recomendadas,
    std::vector<int>& likes,
    std::vector<int>& verMasTarde
) {
    limpiarPantalla();

    while (true) {
        std::cout << "========================================\n";
        std::cout << "        PELICULAS RECOMENDADAS          \n";
        std::cout << "========================================\n";

        if (recomendadas.empty()) {
            std::cout << "  No hay peliculas para recomendar en este momento.\n";
            std::cout << "----------------------------------------\n";
            std::cout << "  [0] Volver al menu principal\n";
            std::cout << "----------------------------------------\n";

            leerOpcion(0, 0);
            return MENU_PRINCIPAL;
        }

        int cantidad = static_cast<int>(recomendadas.size());

        for (int i = 0; i < cantidad; i++) {
            std::cout << "  [" << (i + 1) << "] " << recomendadas[i].titulo << "\n";
        }

        std::cout << "----------------------------------------\n";
        std::cout << "  [1-" << cantidad << "] Ver detalle\n";
        std::cout << "  [0] Volver al menu principal\n";
        std::cout << "----------------------------------------\n";
        std::cout.flush();

        int op = leerOpcion(0, cantidad);

        if (op == 0) {
            return MENU_PRINCIPAL;
        }

        mostrarDetallePelicula(recomendadas[op - 1], likes, verMasTarde);
        limpiarPantalla();
    }
}
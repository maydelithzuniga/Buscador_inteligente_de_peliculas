#pragma once
#include <iostream>
#include <string>
#include <stdexcept>
using namespace std;
inline void limpiarPantalla() {
    cout<<"------------------------------------------------------------"<<endl;
}

inline void pausar() {
    std::cout << "\nPresione Enter para continuar...";
    std::string dummy;
    std::getline(std::cin, dummy);
}

inline int leerOpcion(int min, int max) {
    while (true) {
        std::cout << "Opcion [" << min << "-" << max << "]: ";

        std::string linea;
        std::getline(std::cin, linea);

        if (linea.empty() || linea.find_first_not_of(" \t\r\n") == std::string::npos)
            continue;

        if (!linea.empty() && linea.back() == '\r')
            linea.pop_back();

        try {
            std::size_t pos;
            int valor = std::stoi(linea, &pos);

            if (pos != linea.size()) {
                std::cout << "  [!] Entrada invalida. Solo numeros enteros.\n";
                continue;
            }

            if (valor < min || valor > max) {
                std::cout << "  [!] Fuera de rango. Ingrese entre "
                          << min << " y " << max << ".\n";
                continue;
            }

            return valor;

        } catch (const std::invalid_argument&) {
            std::cout << "  [!] Entrada invalida. Ingrese un numero.\n";
        } catch (const std::out_of_range&) {
            std::cout << "  [!] Numero demasiado grande. Intente de nuevo.\n";
        }
    }
}

inline std::string leerTexto() {
    while (true) {
        std::string texto;
        std::getline(std::cin, texto);

        if (!texto.empty() && texto.back() == '\r')
            texto.pop_back();

        const std::string ws = " \t\r\n";
        size_t inicio = texto.find_first_not_of(ws);
        size_t fin    = texto.find_last_not_of(ws);

        if (inicio == std::string::npos) {
            std::cout << "  [!] El texto no puede estar vacio: ";
            continue;
        }

        return texto.substr(inicio, fin - inicio + 1);
    }
}

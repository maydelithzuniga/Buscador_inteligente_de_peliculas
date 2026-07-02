#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <memory>

template <typename T>
struct NodoSufijo {
    std::unordered_map<char, std::shared_ptr<NodoSufijo<T>>> hijos;
    std::string etiqueta;
    std::vector<T> items_ids; 

    NodoSufijo() = default;
    explicit NodoSufijo(const std::string& lbl) : etiqueta(lbl) {}
};

template <typename T>
class SuffixTree {
private:
    std::shared_ptr<NodoSufijo<T>> raiz;

public:
    SuffixTree() : raiz(std::make_shared<NodoSufijo<T>>()) {}

    inline void insertar(const std::string& token, const T& id) {
        for (size_t i = 0; i < token.size(); ++i)
            _insertar(raiz, token.substr(i), id);
    }

    inline void insertarPalabraCompleta(const std::string& token, const T& id) {
        if (!token.empty()) {
            _insertar(raiz, token, id);
        }
    }

    inline std::vector<T> buscar(const std::string& patron) const {
        std::vector<T> resultado;
        auto nodo = _navegar(raiz, patron);
        if (nodo) {
            _recolectar(nodo, resultado);
        }
        return resultado;
    }

private:
    inline void _insertar(std::shared_ptr<NodoSufijo<T>> nodo, const std::string& sufijo, const T& id) {
        char primer_char = sufijo[0];
        auto it = nodo->hijos.find(primer_char);

        if (it == nodo->hijos.end()) {
            auto nuevo_nodo = std::make_shared<NodoSufijo<T>>(sufijo);
            _anotar(nuevo_nodo, id);
            nodo->hijos[primer_char] = nuevo_nodo;
            return;
        }

        auto hijo = it->second;
        size_t coincide = _prefijo_comun(hijo->etiqueta, sufijo);

        if (coincide < hijo->etiqueta.size()) {
            std::string sufijo_hijo_restante = hijo->etiqueta.substr(coincide);
            hijo->etiqueta = hijo->etiqueta.substr(0, coincide);

            auto nodo_intermedio = std::make_shared<NodoSufijo<T>>(sufijo_hijo_restante);
            nodo_intermedio->hijos = std::move(hijo->hijos);
            nodo_intermedio->items_ids = std::move(hijo->items_ids);

            hijo->hijos.clear();
            hijo->hijos[sufijo_hijo_restante[0]] = nodo_intermedio;
            _anotar(hijo, id);

            if (coincide < sufijo.size()) {
                std::string nuevo_sufijo_resto = sufijo.substr(coincide);
                auto nuevo_nodo = std::make_shared<NodoSufijo<T>>(nuevo_sufijo_resto);
                _anotar(nuevo_nodo, id);
                hijo->hijos[nuevo_sufijo_resto[0]] = nuevo_nodo;
            } else {
                _anotar(hijo, id);
            }
            return;
        }

        _anotar(hijo, id);
        if (coincide < sufijo.size()) {
            _insertar(hijo, sufijo.substr(coincide), id);
        }
    }

    inline std::shared_ptr<NodoSufijo<T>> _navegar(std::shared_ptr<NodoSufijo<T>> nodo, const std::string& patron) const {
        if (patron.empty()) return nodo;
        auto it = nodo->hijos.find(patron[0]);
        if (it == nodo->hijos.end()) return nullptr;

        auto hijo = it->second;
        size_t coincide = _prefijo_comun(hijo->etiqueta, patron);

        if (coincide < patron.size() && coincide == hijo->etiqueta.size())
            return _navegar(hijo, patron.substr(coincide));

        if (coincide >= patron.size())
            return hijo;

        return nullptr;
    }

    inline void _recolectar(std::shared_ptr<NodoSufijo<T>> nodo, std::vector<T>& out) const {
        for (const T& id : nodo->items_ids) out.push_back(id);
        for (auto& [_, hijo] : nodo->hijos) _recolectar(hijo, out);
    }

    inline static size_t _prefijo_comun(const std::string& a, const std::string& b) {
        size_t i = 0;
        while (i < a.size() && i < b.size() && a[i] == b[i]) ++i;
        return i;
    }

    inline static void _anotar(std::shared_ptr<NodoSufijo<T>> nodo, const T& id) {
        if (nodo->items_ids.empty() || nodo->items_ids.back() != id) {
            nodo->items_ids.push_back(id);
        }
    }
};

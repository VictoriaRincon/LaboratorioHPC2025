#include "../include/calculador_costos.hpp"
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <sstream>

CalculadorCostos::CalculadorCostos() : mostrarDetalles(false), horaInicialActual(0), cachehits(0), cachemisses(0) {
    inicializarTransiciones();
}

void CalculadorCostos::inicializarTransiciones() {
    // Definir transiciones válidas según las reglas del problema
    transicionesValidas[Estado::ON_CALIENTE] = {Estado::OFF_CALIENTE, Estado::ON_CALIENTE};
    transicionesValidas[Estado::OFF_CALIENTE] = {Estado::ON_TIBIO, Estado::OFF_TIBIO};
    transicionesValidas[Estado::ON_TIBIO] = {Estado::ON_CALIENTE, Estado::OFF_CALIENTE};
    transicionesValidas[Estado::OFF_TIBIO] = {Estado::ON_FRIO, Estado::OFF_FRIO};
    transicionesValidas[Estado::ON_FRIO] = {Estado::ON_TIBIO, Estado::OFF_TIBIO};
    transicionesValidas[Estado::OFF_FRIO] = {Estado::ON_FRIO, Estado::OFF_FRIO};
}

// Método principal optimizado con caché integrado al backtracking
ResultadoOptimizacion CalculadorCostos::resolverConPatrones(const Escenario& escenario) {
    limpiarMemoizacion();
    cachehits = 0;
    cachemisses = 0;
    horaInicialActual = escenario.getTamaño() - 1;
    
    const auto& datos = escenario.getEnergiasEolicas();
    
    if (mostrarDetalles) {
        std::cout << "=== RESOLVIENDO CON CACHÉ INTEGRADO ===" << std::endl;
        std::cout << "Datos: ";
        for (int val : datos) std::cout << val << " ";
        std::cout << std::endl;
    }
    
    // Intentar usar caché primero para pre-resolver subsecuencias conocidas
    bool usoCacheExistente = aplicarCacheExistente(escenario);
    
    if (usoCacheExistente) {
        // Cache hit: se encontraron patrones existentes
        cachehits++;
        if (mostrarDetalles) {
            std::cout << "Aplicando patrones existentes del caché..." << std::endl;
        }
    } else {
        // Cache miss: no se encontraron patrones reutilizables
        cachemisses++;
    }
    
    // Resolver normalmente (si no se pudo usar caché completo)
    auto resultado = resolver(escenario);
    
    if (resultado.solucionValida) {
        // Extraer y guardar nuevos patrones de la solución encontrada
        extraerNuevosPatrones(datos, resultado.estados);
    }
    
    if (mostrarDetalles) {
        std::cout << "Cache hits: " << cachehits << ", misses: " << cachemisses << std::endl;
        std::cout << "Patrones en caché: " << cachePatrones.size() << std::endl;
    }
    
    return resultado;
}

// Intentar aplicar patrones existentes del caché antes de resolver
bool CalculadorCostos::aplicarCacheExistente(const Escenario& escenario) {
    const auto& datos = escenario.getEnergiasEolicas();
    bool usoCacheAlguno = false;
    
    // Detectar todos los patrones válidos en los datos
    for (int i = 0; i < static_cast<int>(datos.size()); ++i) {
        if (datos[i] == 0) {
            auto patronInfo = detectarPatronCompleto(datos, i);
            
            if (patronInfo.first.size() > 1 && estaEnCache(patronInfo.first)) {
                if (mostrarDetalles) {
                    std::cout << "Patrón encontrado en caché: " << patronAString(patronInfo.first) 
                              << " (pos " << i << "-" << patronInfo.second << ")" << std::endl;
                }
                
                // Pre-memoizar este patrón usando la solución del caché
                prememoizarPatron(patronInfo.first, i, patronInfo.second, escenario);
                usoCacheAlguno = true;
                
                // Saltar al final del patrón
                i = patronInfo.second;
            }
        }
    }
    
    return usoCacheAlguno;
}

// Pre-memoizar un patrón desde el caché
void CalculadorCostos::prememoizarPatron(const std::vector<int>& patron, int horaInicio, int horaFin, const Escenario& escenario) {
    auto solucionPatron = obtenerDeCache(patron);
    
    if (solucionPatron.empty()) return;
    
    // Memoizar cada estado del patrón
    for (int i = 0; i < static_cast<int>(solucionPatron.size()); ++i) {
        int horaGlobal = horaInicio + i;
        Estado estadoActual = solucionPatron[i];
        
        // Calcular costo acumulado desde esta posición hasta el final del patrón
        double costoDesdeAqui = 0.0;
        for (int j = i; j < static_cast<int>(solucionPatron.size()); ++j) {
            costoDesdeAqui += getCostoEstado(solucionPatron[j]);
        }
        
        // Determinar el estado anterior en el patrón (si existe)
        Estado estadoAnterior = (i > 0) ? solucionPatron[i-1] : Estado::OFF_FRIO;
        
        // Memoizar este estado
        auto clave = std::make_pair(horaGlobal, estadoActual);
        memo[clave] = EstadoMemo(costoDesdeAqui, true, estadoAnterior);
    }
    
    if (mostrarDetalles) {
        std::cout << "Pre-memoizado patrón " << patronAString(patron) 
                  << " desde hora " << horaInicio << " hasta " << horaFin << std::endl;
    }
}

// Algoritmo recursivo modificado con caché integrado (versión simplificada)
EstadoMemo CalculadorCostos::resolverRecursivoConCache(int hora, Estado estadoLlegada, const Escenario& escenario) {
    // Esta función ahora es solo un wrapper del algoritmo original
    // La lógica del caché se maneja en el nivel superior
    return resolverRecursivo(hora, estadoLlegada, escenario);
}

// Detectar patrón completo desde una hora con 0 hasta el siguiente 0
std::pair<std::vector<int>, int> CalculadorCostos::detectarPatronCompleto(const std::vector<int>& datos, int horaInicio) {
    std::vector<int> patron;
    int horaActual = horaInicio;
    
    // Debe empezar en 0
    if (datos[horaActual] != 0) {
        return {patron, horaInicio};
    }
    
    patron.push_back(0);
    horaActual++;
    
    // Buscar hasta el siguiente 0 o el final
    while (horaActual < static_cast<int>(datos.size()) && datos[horaActual] != 0) {
        patron.push_back(datos[horaActual]);
        horaActual++;
    }
    
    // Si encontramos otro 0, incluirlo
    if (horaActual < static_cast<int>(datos.size()) && datos[horaActual] == 0) {
        patron.push_back(0);
        return {patron, horaActual};
    }
    
    // Si llegamos al final sin encontrar otro 0, no es un patrón válido
    return {std::vector<int>(), horaInicio};
}

// Extraer nuevos patrones de la solución completa y agregarlos al caché
void CalculadorCostos::extraerNuevosPatrones(const std::vector<int>& datos, const std::vector<Estado>& solucion) {
    if (mostrarDetalles) {
        std::cout << "Extrayendo nuevos patrones..." << std::endl;
    }
    
    for (int i = 0; i < static_cast<int>(datos.size()); ++i) {
        if (datos[i] == 0) {
            auto patronInfo = detectarPatronCompleto(datos, i);
            
            if (patronInfo.first.size() > 1 && !estaEnCache(patronInfo.first)) {
                // Extraer la subsolucion correspondiente
                int horaFin = patronInfo.second;
                std::vector<Estado> subsolucion(solucion.begin() + i, solucion.begin() + horaFin + 1);
                
                guardarEnCache(patronInfo.first, subsolucion);
                
                if (mostrarDetalles) {
                    std::cout << "Nuevo patrón guardado: " << patronAString(patronInfo.first) 
                              << " (pos " << i << "-" << horaFin << ")" << std::endl;
                }
                
                // Saltar al final del patrón para evitar duplicados
                i = horaFin;
            }
        }
    }
}

// Funciones de caché (sin cambios)
bool CalculadorCostos::estaEnCache(const std::vector<int>& patron) const {
    return cachePatrones.find(patron) != cachePatrones.end();
}

std::vector<Estado> CalculadorCostos::obtenerDeCache(const std::vector<int>& patron) const {
    auto it = cachePatrones.find(patron);
    return (it != cachePatrones.end()) ? it->second : std::vector<Estado>();
}

void CalculadorCostos::guardarEnCache(const std::vector<int>& patron, const std::vector<Estado>& solucion) {
    cachePatrones[patron] = solucion;
}

void CalculadorCostos::limpiarCachePatrones() {
    cachePatrones.clear();
    cachehits = 0;
    cachemisses = 0;
}

int CalculadorCostos::getTamañoCachePatrones() const {
    return static_cast<int>(cachePatrones.size());
}

void CalculadorCostos::mostrarEstadisticasCache() const {
    std::cout << "\n=== ESTADÍSTICAS DE CACHÉ ===" << std::endl;
    std::cout << "Patrones almacenados: " << cachePatrones.size() << std::endl;
    std::cout << "Cache hits: " << cachehits << std::endl;
    std::cout << "Cache misses: " << cachemisses << std::endl;
    
    if (cachehits + cachemisses > 0) {
        double hitRate = static_cast<double>(cachehits) / (cachehits + cachemisses) * 100.0;
        std::cout << "Hit rate: " << std::fixed << std::setprecision(1) << hitRate << "%" << std::endl;
    }
    
    if (mostrarDetalles && !cachePatrones.empty()) {
        std::cout << "\nPatrones en caché:" << std::endl;
        for (const auto& par : cachePatrones) {
            std::cout << "  " << patronAString(par.first) << " -> [";
            for (size_t i = 0; i < par.second.size(); ++i) {
                std::cout << estadoAString(par.second[i]);
                if (i < par.second.size() - 1) std::cout << ", ";
            }
            std::cout << "]" << std::endl;
        }
    }
}

int CalculadorCostos::getCacheHits() const {
    return cachehits;
}

int CalculadorCostos::getCacheMisses() const {
    return cachemisses;
}

void CalculadorCostos::resetearEstadisticasCache() {
    cachehits = 0;
    cachemisses = 0;
}

std::string CalculadorCostos::patronAString(const std::vector<int>& patron) const {
    std::ostringstream oss;
    oss << "{";
    for (size_t i = 0; i < patron.size(); ++i) {
        oss << patron[i];
        if (i < patron.size() - 1) oss << ",";
    }
    oss << "}";
    return oss.str();
}

// Funciones que quedan sin cambios del algoritmo original...
ResultadoOptimizacion CalculadorCostos::resolver(const Escenario& escenario) {
    return resolver(escenario, escenario.getTamaño() - 1, 0);
}

ResultadoOptimizacion CalculadorCostos::resolver(const Escenario& escenario, int horaInicial, int horaFinal) {
    limpiarMemoizacion();
    horaInicialActual = horaInicial;
    
    if (horaInicial < horaFinal || horaInicial >= escenario.getTamaño()) {
        return {std::vector<Estado>(), 0.0, false, 0};
    }
    
    if (mostrarDetalles) {
        std::cout << "Resolviendo desde hora " << horaInicial << " hasta hora " << horaFinal << std::endl;
    }
    
    // Probar todos los estados finales posibles
    double mejorCosto = std::numeric_limits<double>::infinity();
    Estado mejorEstadoFinal = Estado::OFF_FRIO;
    bool solucionEncontrada = false;
    
    for (int i = 0; i < 6; ++i) {
        Estado estadoFinal = static_cast<Estado>(i);
        EstadoMemo resultado = resolverRecursivo(horaInicial, estadoFinal, escenario);
        
        if (resultado.valido && resultado.costo < mejorCosto) {
            mejorCosto = resultado.costo;
            mejorEstadoFinal = estadoFinal;
            solucionEncontrada = true;
        }
    }
    
    if (!solucionEncontrada) {
        return {std::vector<Estado>(), 0.0, false, 0};
    }
    
    // Reconstruir la solución
    std::vector<Estado> solucion = reconstruirSolucion(escenario, horaInicial);
    
    return {solucion, mejorCosto, true, horaInicial - horaFinal + 1};
}

EstadoMemo CalculadorCostos::resolverRecursivo(int hora, Estado estadoLlegada, const Escenario& escenario) {
    // Caso base: hora < 0
    if (hora < 0) {
        return EstadoMemo(0.0, true, Estado::OFF_FRIO);
    }
    
    // Verificar si ya está memoizado
    auto clave = std::make_pair(hora, estadoLlegada);
    if (memo.find(clave) != memo.end()) {
        return memo[clave];
    }
    
    double mejorCosto = std::numeric_limits<double>::infinity();
    Estado mejorEstadoAnterior = Estado::OFF_FRIO;
    bool solucionValida = false;
    
    // Obtener estados que pueden transicionar a estadoLlegada
    std::vector<Estado> estadosAnteriores = getEstadosQueVanA(estadoLlegada);
    
    for (Estado estadoAnterior : estadosAnteriores) {
        bool esValidoParaEstaHora = true;
        
        // Verificar restricción de energía eólica
        if (!escenario.esEolicaSuficiente(hora)) {
            // Requiere generación (ON/CALIENTE)
            esValidoParaEstaHora = puedeGenerarEnergia(estadoLlegada);
        }
        // Si hay energía eólica suficiente, cualquier estado es válido
        
        if (esValidoParaEstaHora) {
            EstadoMemo resultado = resolverRecursivo(hora - 1, estadoAnterior, escenario);
            
            if (resultado.valido) {
                double costoTotal = resultado.costo + getCostoEstado(estadoLlegada);
                if (costoTotal < mejorCosto) {
                    mejorCosto = costoTotal;
                    mejorEstadoAnterior = estadoAnterior;
                    solucionValida = true;
                }
            }
        }
    }
    
    EstadoMemo resultado(mejorCosto, solucionValida, mejorEstadoAnterior);
    memo[clave] = resultado;
    return resultado;
}

std::vector<Estado> CalculadorCostos::getEstadosQueVanA(Estado estadoDestino) const {
    std::vector<Estado> estados;
    
    for (const auto& par : transicionesValidas) {
        const auto& transiciones = par.second;
        if (std::find(transiciones.begin(), transiciones.end(), estadoDestino) != transiciones.end()) {
            estados.push_back(par.first);
        }
    }
    
    return estados;
}

bool CalculadorCostos::esTransicionValida(Estado desde, Estado hacia) const {
    auto it = transicionesValidas.find(desde);
    if (it == transicionesValidas.end()) {
        return false;
    }
    
    const auto& transiciones = it->second;
    return std::find(transiciones.begin(), transiciones.end(), hacia) != transiciones.end();
}

std::vector<Estado> CalculadorCostos::reconstruirSolucion(const Escenario& escenario, int horaInicial) {
    std::vector<Estado> solucion;
    
    // Encontrar el mejor estado final
    double mejorCosto = std::numeric_limits<double>::infinity();
    Estado estadoActual = Estado::OFF_FRIO;
    
    for (int i = 0; i < 6; ++i) {
        Estado estado = static_cast<Estado>(i);
        auto clave = std::make_pair(horaInicial, estado);
        if (memo.find(clave) != memo.end() && memo[clave].valido) {
            if (memo[clave].costo < mejorCosto) {
                mejorCosto = memo[clave].costo;
                estadoActual = estado;
            }
        }
    }
    
    // Reconstruir hacia atrás
    for (int hora = horaInicial; hora >= 0; --hora) {
        solucion.push_back(estadoActual);
        
        if (hora > 0) {
            auto clave = std::make_pair(hora, estadoActual);
            if (memo.find(clave) != memo.end()) {
                estadoActual = memo[clave].estadoAnterior;
            }
        }
    }
    
    // Invertir para tener orden cronológico
    std::reverse(solucion.begin(), solucion.end());
    return solucion;
}

void CalculadorCostos::limpiarMemoizacion() {
    memo.clear();
}

void CalculadorCostos::setMostrarDetalles(bool mostrar) {
    mostrarDetalles = mostrar;
}

std::string CalculadorCostos::estadoAString(Estado estado) {
    switch (estado) {
        case Estado::ON_CALIENTE: return "ON/CALIENTE";
        case Estado::ON_TIBIO: return "ON/TIBIO";
        case Estado::ON_FRIO: return "ON/FRIO";
        case Estado::OFF_CALIENTE: return "OFF/CALIENTE";
        case Estado::OFF_TIBIO: return "OFF/TIBIO";
        case Estado::OFF_FRIO: return "OFF/FRIO";
        default: return "DESCONOCIDO";
    }
}

double CalculadorCostos::getCostoEstado(Estado estado) {
    switch (estado) {
        case Estado::ON_CALIENTE: return 5.0;
        case Estado::ON_TIBIO: return 2.5;
        case Estado::ON_FRIO: return 1.0;
        case Estado::OFF_CALIENTE:
        case Estado::OFF_TIBIO:
        case Estado::OFF_FRIO:
        default: return 0.0;
    }
}

bool CalculadorCostos::puedeGenerarEnergia(Estado estado) {
    return estado == Estado::ON_CALIENTE;
}

// Métodos obsoletos que mantengo por compatibilidad pero no se usan
std::vector<SubsecuenciaInfo> CalculadorCostos::identificarSubsecuencias(const std::vector<int>& datos) {
    return std::vector<SubsecuenciaInfo>();
}

std::vector<Estado> CalculadorCostos::resolverSubsecuencia(const std::vector<int>& patron) {
    return std::vector<Estado>();
}

void CalculadorCostos::extraerYGuardarPatrones(const std::vector<int>& patronOriginal, const std::vector<Estado>& solucion) {
    // Método obsoleto
}

std::vector<Estado> CalculadorCostos::ensamblarSolucionCompleta(const std::vector<SubsecuenciaInfo>& subsecuencias, 
                                                               const std::vector<int>& datosOriginales) {
    return std::vector<Estado>();
} 
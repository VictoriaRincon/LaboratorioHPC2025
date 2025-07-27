# Sistema Completo de Análisis de Máquina de Estados con Transiciones

## 🎯 Nueva Funcionalidad: Análisis de Transiciones Prender/Apagar

El sistema ahora incluye una **columna de transiciones** que muestra cuándo la máquina se prende y apaga durante el día.

### Formato de la Columna Transiciones:

| Valor | Significado | Ejemplo |
|-------|-------------|---------|
| `""` (vacío) | Nunca se prendió la máquina | La máquina siempre OFF |
| `0-23` | Prendida desde hora 0 hasta 23 | Siempre prendida |
| `6-23` | Prender hora 6, mantener hasta 23 | Prender en H6 |
| `2-5-8-15` | Prender H2, apagar H5, prender H8, apagar H15 | Múltiples ciclos |

### Estados considerados "prendido":
- `ON/CALIENTE` (genera energía)
- `ON/TIBIO` (no genera pero consume)
- `ON/FRIO` (no genera pero consume)

### Estados considerados "apagado":
- `OFF/CALIENTE`, `OFF/TIBIO`, `OFF/FRIO` (sin consumo)

## 📊 Formato CSV Actualizado

```csv
CombinacionID,PatronEolica,CostoTotal,SolucionValida,HorasCriticas,Transiciones
255,000000000000000011111111,83.50,SI,16,6-23
3,000000000000000000000011,112.50,SI,22,1-23
6,000000000000000000000110,112.50,SI,22,0-1-2-23
```

## 🚀 Uso del Sistema

### 1. Análisis Masivo con Transiciones
```bash
./scripts/ejecutar_analisis.sh
```

### 2. Análisis de Resultados con Transiciones
```bash
./scripts/analizar_resultados.sh resultados/archivo.csv
```

### 3. Análisis Individual Detallado
```bash
echo "000000000000000011111111" | ./analizador_individual
```

## 📈 Insights de Transiciones Encontrados

### Mejores Patrones (Costo 83.50):
- **Transición óptima**: `[6-23]` - Prender en hora 6 y mantener
- **Estrategia**: Usar estados OFF durante energía eólica (H0-7), luego prender
- **Eficiencia**: 88.9% energética (16/18 horas generando vs consumiendo)

### Distribución de Patrones (muestra 1,000 casos):
- **22%** siempre prendidos (`0-23`)
- **78%** con transiciones inteligentes
- **Patrón más común**: `0-23` (220 casos), `1-23` (84 casos)

### Estadísticas de Transiciones:
```
🔄 ANÁLISIS DE TRANSICIONES:
   Nunca prendido: 0 casos (0%)
   Siempre prendido (0-23): 220 casos (22.0%)
   Con transiciones: 780 casos (78.0%)

🏅 MEJORES TRANSICIONES:
   [6-23]: Costo 83.50 (múltiples casos)
   [5-23]: Costo 88.50
   [0-1-6-23]: Costo 88.50
```

## 🔍 Comandos de Análisis de Transiciones

### Filtrar por patrones específicos:
```bash
# Casos que nunca se prenden
awk -F, 'NR>1 && $6=="" {print}' archivo.csv

# Casos siempre prendidos
awk -F, 'NR>1 && $6=="0-23" {print}' archivo.csv

# Buscar transición específica
grep "6-23" archivo.csv

# Contar patrones únicos
awk -F, 'NR>1 {print $6}' archivo.csv | sort | uniq -c | sort -nr
```

### Análisis estadístico:
```bash
# Mejores transiciones por costo
sort -t, -k3 -n archivo.csv | head -10

# Distribución de número de transiciones
awk -F, 'NR>1 {print gsub(/-/, "-", $6)}' archivo.csv | sort -n | uniq -c
```

## 🎯 Ejemplo de Análisis Completo

### Patrón: `000000000000000011111111` (Energía eólica H0-7)

**Resultado del análisis masivo:**
- Costo: 83.50
- Transiciones: `[6-23]`
- Horas críticas: 16

**Análisis individual detallado:**
```
Secuencia de estados:
H0-5: OFF (aprovecha eólica, costo 0)
H6: ON/FRIO (preparación, costo 1.0)
H7: ON/TIBIO (transición, costo 2.5)
H8-23: ON/CALIENTE (generación, costo 5.0×16 = 80.0)

Total: 0 + 1.0 + 2.5 + 80.0 = 83.50 ✓
```

**Interpretación de transiciones:**
- `6-23` significa: "Empezar a prender en hora 6, estar completamente activo para generar desde hora 8"
- La máquina hace una transición gradual OFF → ON/FRIO → ON/TIBIO → ON/CALIENTE

## 🔧 Arquitectura del Sistema

```
📁 Componentes principales:
├── demo_analisis_con_transiciones     # Motor masivo con transiciones
├── analizador_individual              # Análisis detallado caso a caso
├── scripts/ejecutar_analisis.sh       # Interfaz para análisis masivo
├── scripts/analizar_resultados.sh     # Análisis estadístico de transiciones
└── resultados/                        # Archivos CSV con transiciones
```

## 🏆 Beneficios del Análisis de Transiciones

1. **Visibilidad operativa**: Ver exactamente cuándo prender/apagar
2. **Optimización energética**: Identificar patrones eficientes
3. **Análisis de costos**: Correlacionar transiciones con costos
4. **Planificación**: Usar patrones óptimos para nuevos escenarios
5. **Validación**: Verificar que las transiciones respetan la máquina de estados

## 📊 Próximos Análisis Recomendados

1. **Análisis de 100,000 combinaciones** para patrones más robustos
2. **Correlación entre posición de eólica y transiciones** óptimas
3. **Análisis de sensibilidad** cambiando costos de mantenimiento
4. **Optimización de arranque** para minimizar transiciones

El sistema está **completamente optimizado** para análisis masivo con visibilidad completa de patrones de operación. 🎉

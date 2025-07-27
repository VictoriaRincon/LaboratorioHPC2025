// Línea problemática en analizador_individual.cpp:
// double energia_eolica = patron_eolica[23-hora] ? 500.0 : 0.0;  // MAL
// Debería ser:
// double energia_eolica = patron_eolica[hora] ? 500.0 : 0.0;     // BIEN

// Voy a corregir el archivo

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#if defined(__GNUC__) && !defined(__clang__)
#define BENCHMARK_O0 __attribute__((optimize("O0"), noinline))
#define BENCHMARK_O2 __attribute__((optimize("O2"), noinline))
#define BENCHMARK_O3 __attribute__((optimize("O3"), noinline))
#else
#error "Este benchmark requiere GCC para ejecutar las variantes reales -O0, -O2 y -O3."
#endif

struct Statistics {
    double mean;
    double median;
    double min_val;
    double max_val;
};

struct BenchmarkResult {
    std::string code;
    std::string configuration;
    std::string input;
    std::string observation;
    std::vector<double> times;
    Statistics stats;
    double speedup;
};

volatile std::uint64_t integer_sink = 0;
volatile double float_sink = 0.0;

Statistics calculate_stats(const std::vector<double>& times) {
    Statistics stats{};
    stats.mean = std::accumulate(times.begin(), times.end(), 0.0) / times.size();

    std::vector<double> sorted_times = times;
    std::sort(sorted_times.begin(), sorted_times.end());
    stats.min_val = sorted_times.front();
    stats.max_val = sorted_times.back();

    const std::size_t n = sorted_times.size();
    stats.median = n % 2 == 0
        ? (sorted_times[n / 2 - 1] + sorted_times[n / 2]) / 2.0
        : sorted_times[n / 2];
    return stats;
}

template <typename Test>
std::vector<double> measure(Test test) {
    constexpr int warmups = 2;
    constexpr int repetitions = 10;
    for (int i = 0; i < warmups; ++i) {
        test();
    }

    std::vector<double> times;
    times.reserve(repetitions);
    for (int i = 0; i < repetitions; ++i) {
        times.push_back(test());
    }
    return times;
}

BENCHMARK_O0 double run_cpu_int_o0(std::size_t size) {
    std::vector<std::int64_t> a(size, 3), b(size, 7), c(size);
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t checksum = 0;
    for (std::size_t i = 0; i < size; ++i) {
        c[i] = a[i] * b[i] + static_cast<std::int64_t>(i);
        checksum += static_cast<std::uint64_t>(c[i]);
    }
    const auto end = std::chrono::steady_clock::now();
    integer_sink = checksum;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

BENCHMARK_O0 double run_cpu_float_o0(std::size_t size) {
    std::vector<double> a(size, 3.14159), b(size, 2.71828), c(size);
    const auto start = std::chrono::steady_clock::now();
    double checksum = 0.0;
    for (std::size_t i = 0; i < size; ++i) {
        c[i] = a[i] * b[i] + static_cast<double>(i);
        checksum += c[i];
    }
    const auto end = std::chrono::steady_clock::now();
    float_sink = checksum;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

BENCHMARK_O2 double run_cpu_int_o2(std::size_t size) {
    std::vector<std::int64_t> a(size, 3), b(size, 7), c(size);
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t checksum = 0;
    for (std::size_t i = 0; i < size; ++i) {
        c[i] = a[i] * b[i] + static_cast<std::int64_t>(i);
        checksum += static_cast<std::uint64_t>(c[i]);
    }
    const auto end = std::chrono::steady_clock::now();
    integer_sink = checksum;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

BENCHMARK_O3 double run_cpu_int_o3(std::size_t size) {
    std::vector<std::int64_t> a(size, 3), b(size, 7), c(size);
    const auto start = std::chrono::steady_clock::now();
    std::uint64_t checksum = 0;
    for (std::size_t i = 0; i < size; ++i) {
        c[i] = a[i] * b[i] + static_cast<std::int64_t>(i);
        checksum += static_cast<std::uint64_t>(c[i]);
    }
    const auto end = std::chrono::steady_clock::now();
    integer_sink = checksum;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

BENCHMARK_O2 double run_memory(std::size_t num_elements, bool sequential) {
    std::vector<std::uint64_t> data(num_elements, 1);
    std::vector<std::size_t> indices(num_elements);
    std::iota(indices.begin(), indices.end(), 0);

    if (!sequential) {
        std::mt19937 generator(42);
        std::shuffle(indices.begin(), indices.end(), generator);
    }

    const auto start = std::chrono::steady_clock::now();
    std::uint64_t sum = 0;
    for (std::size_t i = 0; i < num_elements; ++i) {
        sum += data[indices[i]];
    }
    const auto end = std::chrono::steady_clock::now();
    integer_sink = sum;
    return std::chrono::duration<double, std::milli>(end - start).count();
}

BenchmarkResult make_result(
    const std::string& code,
    const std::string& configuration,
    const std::string& input,
    const std::string& observation,
    std::vector<double> times) {
    Statistics stats = calculate_stats(times);
    return {code, configuration, input, observation, std::move(times), stats, 0.0};
}

void write_execution_table(
    const std::string& filename,
    const std::vector<BenchmarkResult>& results) {
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("No se pudo crear " + filename);
    }

    file << "Prueba,Configuracion,Ejecucion,Entrada_Tamano,Tiempo_ms,Observacion\n";
    file << std::fixed << std::setprecision(3);
    for (const auto& result : results) {
        file << result.code << ',' << result.configuration << ",1,"
             << result.input << ',' << result.times.front() << ','
             << result.observation << '\n';
    }
}

void write_summary_table(
    const std::string& filename,
    const std::vector<BenchmarkResult>& results) {
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("No se pudo crear " + filename);
    }

    file << "Prueba,Configuracion,Promedio_ms,Mediana_ms,Minimo_ms,Maximo_ms,Speedup\n";
    file << std::fixed << std::setprecision(3);
    for (const auto& result : results) {
        file << result.code << ',' << result.configuration << ','
             << result.stats.mean << ',' << result.stats.median << ','
             << result.stats.min_val << ',' << result.stats.max_val << ','
             << std::setprecision(2) << result.speedup << "x"
             << std::setprecision(3) << '\n';
    }
}

void write_raw_data(
    const std::string& filename,
    const std::vector<BenchmarkResult>& results) {
    std::ofstream file(filename);
    if (!file) {
        throw std::runtime_error("No se pudo crear " + filename);
    }

    file << "Prueba,Configuracion,Ejecucion,Entrada_Tamano,Tiempo_ms\n";
    file << std::fixed << std::setprecision(3);
    for (const auto& result : results) {
        for (std::size_t i = 0; i < result.times.size(); ++i) {
            file << result.code << ',' << result.configuration << ','
                 << i + 1 << ',' << result.input << ',' << result.times[i] << '\n';
        }
    }
}

void print_tables(const std::vector<BenchmarkResult>& results) {
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "\nTABLA 1. EJECUCION REPRESENTATIVA (EJECUCION 1)\n";
    std::cout << std::left << std::setw(8) << "Prueba"
              << std::setw(30) << "Configuracion"
              << std::setw(12) << "Ejecucion"
              << std::setw(22) << "Entrada/Tamano"
              << std::setw(14) << "Tiempo (ms)"
              << "Observacion\n";
    for (const auto& result : results) {
        std::cout << std::left << std::setw(8) << result.code
                  << std::setw(30) << result.configuration
                  << std::setw(12) << 1
                  << std::setw(22) << result.input
                  << std::setw(14) << result.times.front()
                  << result.observation << '\n';
    }

    std::cout << "\nTABLA 2. RESUMEN Y MEDIDAS ESTADISTICAS (10 EJECUCIONES)\n";
    std::cout << "Speedup = tiempo promedio de referencia / tiempo promedio de la prueba.\n";
    std::cout << "Referencias: A1 para A2, C1 y C2; B1 para B2.\n";
    std::cout << std::left << std::setw(8) << "Prueba"
              << std::setw(30) << "Configuracion"
              << std::right << std::setw(14) << "Promedio"
              << std::setw(14) << "Mediana"
              << std::setw(14) << "Minimo"
              << std::setw(14) << "Maximo"
              << std::setw(12) << "Speedup\n";
    for (const auto& result : results) {
        std::cout << std::left << std::setw(8) << result.code
                  << std::setw(30) << result.configuration
                  << std::right << std::setw(11) << result.stats.mean << " ms"
                  << std::setw(11) << result.stats.median << " ms"
                  << std::setw(11) << result.stats.min_val << " ms"
                  << std::setw(11) << result.stats.max_val << " ms"
                  << std::setw(11) << result.speedup << "x\n";
    }
}

int main(int argc, char* argv[]) {
    try {
        const std::string label = argc > 1 ? argv[1] : "Default_Config";
        constexpr std::size_t cpu_size = 10000000;
        constexpr std::size_t memory_elements = 8388608;
        const std::string cpu_input = "10000000 elem";
        const std::string memory_input = "64 MB (8388608 elem)";

        std::vector<BenchmarkResult> results;
        results.push_back(make_result(
            "A1", "CPU Enteros -O0", cpu_input, "Ejecucion base sin optimizar",
            measure([&]() { return run_cpu_int_o0(cpu_size); })));
        results.push_back(make_result(
            "A2", "CPU Flotantes -O0", cpu_input, "Calculo de punto flotante",
            measure([&]() { return run_cpu_float_o0(cpu_size); })));
        results.push_back(make_result(
            "B1", "Memoria Secuencial -O2", memory_input,
            "Lecturas con localidad espacial",
            measure([&]() { return run_memory(memory_elements, true); })));
        results.push_back(make_result(
            "B2", "Memoria Aleatoria -O2", memory_input,
            "Lecturas en orden aleatorio",
            measure([&]() { return run_memory(memory_elements, false); })));
        results.push_back(make_result(
            "C1", "CPU Enteros -O2", cpu_input,
            "Misma operacion compilada con optimizacion -O2",
            measure([&]() { return run_cpu_int_o2(cpu_size); })));
        results.push_back(make_result(
            "C2", "CPU Enteros -O3", cpu_input,
            "Misma operacion compilada con optimizacion -O3",
            measure([&]() { return run_cpu_int_o3(cpu_size); })));

        const double cpu_baseline = results[0].stats.mean;
        const double memory_baseline = results[2].stats.mean;
        results[0].speedup = 1.0;
        results[1].speedup = cpu_baseline / results[1].stats.mean;
        results[2].speedup = 1.0;
        results[3].speedup = memory_baseline / results[3].stats.mean;
        results[4].speedup = cpu_baseline / results[4].stats.mean;
        results[5].speedup = cpu_baseline / results[5].stats.mean;

        print_tables(results);

        const std::string execution_file = "tabla_1_ejecuciones_" + label + ".csv";
        const std::string summary_file = "tabla_2_resumen_" + label + ".csv";
        const std::string raw_file = "datos_crudos_" + label + ".csv";
        write_execution_table(execution_file, results);
        write_summary_table(summary_file, results);
        write_raw_data(raw_file, results);
        std::cout << "\nArchivos creados:\n  " << execution_file << "\n  "
                  << summary_file << "\n  " << raw_file << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return 1;
    }
}

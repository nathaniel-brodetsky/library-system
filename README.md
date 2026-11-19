# Бібліотечна система

Програмна система керування бібліотечним фондом (C++20, CMake), що використовується як об'єкт дослідження в лабораторних роботах з дисципліни «Якість програмного забезпечення та тестування».

## Збирання і тести

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/library_app
```

| Набір тестів | Лабораторна робота |
|---|---|
| `test_decision_table` | ЛР 3 — таблиці рішень |
| `test_cause_effect` | ЛР 4 — функціональні діаграми |
| `test_equivalence_boundary` | ЛР 5 — класи еквівалентності |
| `test_boundary_values` | ЛР 6 — аналіз граничних значень |

## HTTP-сервер

```bash
./build/library_server --port 8080 --copies 100000 --workers 128
```

| Метод | Шлях | Параметри |
|---|---|---|
| GET | `/health` | — |
| GET | `/stats` | — |
| POST | `/readers` | `id`, `name`, `email`, `date` |
| POST | `/loans` | `reader`, `copy`, `date` |
| POST | `/loans/renew` | `reader`, `copy`, `date` |
| POST | `/returns` | `reader`, `copy`, `date`, `condition` |
| GET | `/readers/{id}/loans` | — |

Дати передаються у форматі `YYYY-MM-DD`, параметри POST — як `application/x-www-form-urlencoded`.

## Навантажувальне тестування (ЛР 7)

Потрібні Java 17+ та Apache JMeter 5.6:

```bash
sudo dnf install -y java-21-openjdk
curl -LO https://archive.apache.org/dist/jmeter/binaries/apache-jmeter-5.6.3.tgz
tar xzf apache-jmeter-5.6.3.tgz -C ~
JMETER=~/apache-jmeter-5.6.3/bin/jmeter PORT=8099 load/run_load.sh
```

Скрипт по черзі запускає сервер і план `load/library_load.jmx` для 10, 50, 100 і 200 потоків по 1000 ітерацій (кількість примірників у фонді сервера дорівнює кількості ітерацій). HTML-звіти JMeter зберігаються в `load/results/report-N`, усі результати пакуються в `load/results.tar.gz`.

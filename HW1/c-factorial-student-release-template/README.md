# Домашняя работа №1. Факториал

Текущий статус тестирования GitHub Actions: [![CI/CD](../../actions/workflows/ci.yaml/badge.svg?branch=main&event=workflow_dispatch)](../../actions/workflows/ci.yaml).

> [!Note]
> Чтобы GitHub Workflow отработал верно, файл с [функцией `main`](https://en.cppreference.com/w/c/language/main_function) должен называться `main.c`.

Вам предоставляется возможность запуска тестов локальным способом. Для этого нужно:

1. Установить [Python](https://www.python.org/).
2. Установить дополнительные библиотеки через [`pip`](https://pypi.org/project/pip/): `difflib`.
3. В корне репозитория запустить `python tests.py <path to executable filename>`.
4. Посмотреть логи тестирования.

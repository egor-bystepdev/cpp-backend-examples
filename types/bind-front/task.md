# Bind front

В вашей команде есть несколько серверов -- для тестирования, для разработчиков, несколько боевых.
Написаны функции тестирования, которые первыми аргументами принимают адреса к серверам
и настройки, а оставшимися -- тестовые данные.

Коллеги просят вас написать функцию `BindFront`, которая бы позволила зафиксировать первые аргументы, чтобы можно было
вызывать функции только с тестовыми данными.

Пример:

```cpp
auto test_server = BindFront(test, test_server_settings, test_server_keys);
auto prod_server = BindFront(test, prod_server_settings);

test_server(data);
prod_server(prod_server_keys, data);
```

Запрещено использовать std::bind\_front и std::bind

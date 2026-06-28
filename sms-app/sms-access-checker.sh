#!/bin/bash

declare -A user_access

# User list
user_access["+19991111111"]=1  # User
user_access["+19992222222"]=2  # Supervisor
user_access["+19993333333"]=3  # Admin

#################################################
#################################################

incoming_number="$2"


if [[ -z "${user_access[$incoming_number]}" ]]; then
    echo "Unknown number"
    exit 1
fi

# 2. Если номер есть, извлекаем его уровень доступа в переменную
level=${user_access[$incoming_number]}

# 3. Маршрутизируем логику скрипта в зависимости от уровня
case $level in
    0)
        echo -n "Access is temprary blocked" | sms-send "$incoming_number"
        # Вызов функций для пользователя
        ;;
    1)
        echo -n "User" | sms-send "$incoming_number"
        # Вызов функций для пользователя
        ;;
    2)
        echo -n "Supervisor" | sms-send "$incoming_number"
        # Вызов функций для модератора
        ;;
    3)
        echo -n "Admin" | sms-send "$incoming_number"
        # Полный доступ
        ;;
    *)
        echo -n "Unknown access level ($level)." | sms-send "$incoming_number"
        ;;
esac

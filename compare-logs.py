import re
import matplotlib.pyplot as plt
import numpy as np

def parse_log(filename):
    indices = []
    times_ms = []
    pattern = re.compile(r'dat\\(\d+)_\d+\.json:\s+time\s+\[([\d.]+)\s+ms\]')

    with open(filename, 'r', encoding='utf-8') as f:
        for line in f:
            match = pattern.search(line)
            if match:
                idx = int(match.group(1))
                t_ms = float(match.group(2))
                indices.append(idx)
                times_ms.append(t_ms)

    # Сортировка по индексу
    sorted_pairs = sorted(zip(indices, times_ms))
    if sorted_pairs:
        indices, times_ms = zip(*sorted_pairs)
        # Перевод в секунды (1 с = 1000 мс)
        times_s = [t / 1000.0 for t in times_ms]
        return list(indices), times_s
    else:
        return [], []

def format_sigfig(value, sigfigs=2):
    """Форматирует число с заданным количеством значащих цифр."""
    if value == 0:
        return "0"
    return f"{value:.{sigfigs}g}"

def main():
    # Чтение данных из обоих логов
    idx_base, time_base = parse_log('log.log')
    idx_par, time_par = parse_log('log-m.log')

    if not idx_base or not idx_par:
        print("Ошибка: не удалось прочитать данные из лог-файлов.")
        return

    # Общие индексы
    common_indices = sorted(set(idx_base) & set(idx_par))
    if not common_indices:
        print("Ошибка: нет общих индексов между файлами.")
        return

    # Словари для быстрого доступа
    time_base_dict = dict(zip(idx_base, time_base))
    time_par_dict = dict(zip(idx_par, time_par))

    x = np.arange(len(common_indices))
    width = 0.35

    base_vals = [time_base_dict[i] for i in common_indices]
    par_vals = [time_par_dict[i] for i in common_indices]

    # Построение
    fig, ax = plt.subplots(figsize=(12, 6))
    rects1 = ax.bar(x - width/2, base_vals, width, label='Базовая версия', color='tab:blue')
    rects2 = ax.bar(x + width/2, par_vals, width, label='Распараллеленная версия', color='tab:orange')

    # Логарифмическая шкала по Y
    ax.set_yscale('log')

    # Оформление
    ax.set_xlabel('Индекс файла (dat\\XXXX_0000.json)', fontsize=12)
    ax.set_ylabel('Время выполнения (секунды) — логарифмическая шкала', fontsize=12)
    ax.set_title('Сравнение времени выполнения', fontsize=14)
    ax.set_xticks(x)
    ax.set_xticklabels(common_indices, rotation=45)
    ax.legend()

    # Подписи значений над столбцами (округление до 2 значащих цифр)
    def autolabel(rects):
        for rect in rects:
            height = rect.get_height()
            # Форматируем с двумя значащими цифрами, добавляем " с"
            label = format_sigfig(height, 2) + " с"
            ax.annotate(label,
                        xy=(rect.get_x() + rect.get_width() / 2, height),
                        xytext=(0, 3),
                        textcoords="offset points",
                        ha='center', va='bottom', fontsize=8)

    autolabel(rects1)
    autolabel(rects2)

    plt.tight_layout()
    plt.show()

if __name__ == '__main__':
    main()

#include "parameter_help.h"

namespace {

ParameterHelp codingHelp(HelpTopic topic)
{
    switch (topic) {
    case HelpTopic::Code:
        return {QStringLiteral("Код"),
            QStringLiteral("Готовый циклический код из таблицы методички. Запись (n, k), t: n — длина кодового "
                           "слова, k — число бит данных, t — сколько ошибок код мог бы исправить."),
            QStringLiteral("любой код из списка или «Свой многочлен»; при ручном вводе n и g(x) список сам "
                           "переключится на совпадающий код."),
            QStringLiteral("(15, 11), t = 1: 10011 — простой код с 4 проверочными битами."),
            QStringLiteral("чем больше проверочных бит h = n − k, тем реже ложный приём, но выше избыточность I.")};
    case HelpTopic::CodeLength:
        return {QStringLiteral("Длина n"),
            QStringLiteral("Сколько бит в одном передаваемом блоке: k бит данных плюс h проверочных."),
            QStringLiteral("целое число от 3 до 64; оно должно быть больше старшей степени g(x)."),
            QStringLiteral("15, 31 или 63 для кодов из таблицы."),
            QStringLiteral("при той же вероятности ошибки p длинный блок искажается чаще: Pв = (1 − p)<sup>n</sup>.")};
    default:
        return {QStringLiteral("g(x)"),
            QStringLiteral("Образующий многочлен кода. Передатчик делит блок на g(x) и дописывает остаток, "
                           "приёмник проверяет, что остаток от деления равен нулю."),
            QStringLiteral("двоичная запись от старшей степени (10011) или список степеней в скобках ([4;1;0]); "
                           "первый и последний коэффициенты равны 1, старшая степень меньше n."),
            QStringLiteral("10011 = [4;1;0] = x<sup>4</sup> + x + 1, тогда h = 4."),
            QStringLiteral("степень g(x) задаёт число проверочных бит h; удачный многочлен даёт большое кодовое "
                           "расстояние d и малую Pлож.")};
    }
}

ParameterHelp bscHelp()
{
    return {QStringLiteral("Вероятность ошибки p"),
        QStringLiteral("Вероятность того, что канал исказит один бит. В ДСК биты искажаются независимо друг от друга."),
        QStringLiteral("0 &lt; p ≤ 0,5; можно вводить с запятой или с точкой."),
        QStringLiteral("0,005 — в среднем один искажённый бит на 200."),
        QStringLiteral("чем больше p, тем меньше Pв и больше Pобн и Pлож.")};
}

ParameterHelp gilbertHelp(HelpTopic topic)
{
    switch (topic) {
    case HelpTopic::GoodToBad:
        return {QStringLiteral("P_ХП"),
            QStringLiteral("Вероятность того, что после очередного бита канал перейдёт из хорошего состояния в "
                           "плохое. Задаёт, как часто начинаются пачки ошибок."),
            QStringLiteral("0 &lt; P_ХП &lt; 1."),
            QStringLiteral("0,001 — пачка начинается в среднем раз на 1000 бит."),
            QStringLiteral("чем больше P_ХП, тем больше доля плохого состояния P_П и средняя вероятность ошибки.")};
    case HelpTopic::BadToGood:
        return {QStringLiteral("P_ПХ"),
            QStringLiteral("Вероятность того, что после очередного бита канал вернётся из плохого состояния в "
                           "хорошее. Средняя длина пачки равна 1/P_ПХ бит."),
            QStringLiteral("0 &lt; P_ПХ &lt; 1."),
            QStringLiteral("0,1 — пачка длится в среднем 10 бит."),
            QStringLiteral("чем меньше P_ПХ, тем длиннее пачки; пачки длиннее h бит код может пропустить.")};
    case HelpTopic::ErrorInBad:
        return {QStringLiteral("p_П"),
            QStringLiteral("Вероятность ошибки бита, пока канал находится в плохом состоянии."),
            QStringLiteral("0 &lt; p_П ≤ 0,5."),
            QStringLiteral("0,5 — внутри пачки каждый бит искажается с вероятностью 1/2."),
            QStringLiteral("вместе с p_Х определяет среднюю вероятность ошибки p = p_Х·P_Х + p_П·P_П.")};
    default:
        return {QStringLiteral("p_Х"),
            QStringLiteral("Вероятность ошибки бита, пока канал находится в хорошем состоянии."),
            QStringLiteral("0 ≤ p_Х ≤ 0,5; ноль означает, что вне пачек ошибок нет."),
            QStringLiteral("0 — ошибки бывают только в пачках."),
            QStringLiteral("даже малое p_Х добавляет одиночные ошибки между пачками.")};
    }
}

ParameterHelp controlHelp(HelpTopic topic)
{
    switch (topic) {
    case HelpTopic::StopMode:
        return {QStringLiteral("Остановка"),
            QStringLiteral("Когда заканчивать опыт: после передачи N блоков или когда наберётся заданное число "
                           "ошибочных блоков (Nобн + Nлож)."),
            QStringLiteral("«по числу блоков N» или «по числу ошибочных блоков»."),
            QStringLiteral("по числу ошибочных блоков с порогом 1000 — удобно при очень малых p."),
            QStringLiteral("второй режим даёт одинаковую точность оценок при любом уровне помех; N в нём — "
                           "предел, после которого опыт всё равно остановится.")};
    case HelpTopic::BlockLimit:
        return {QStringLiteral("Число блоков N"),
            QStringLiteral("Сколько блоков передать за опыт. В режиме остановки по ошибочным блокам это "
                           "наибольшее допустимое число блоков."),
            QStringLiteral("целое число от 1 до 1 000 000 000; пробелы между разрядами допускаются."),
            QStringLiteral("3 000 000 — достаточно, чтобы проверить требование Pлож ≤ 10⁻⁶."),
            QStringLiteral("чем больше N, тем точнее оценки и дольше опыт; если ложных приёмов не было, "
                           "Pлож &lt; 3/N.")};
    case HelpTopic::ErrorLimit:
        return {QStringLiteral("Ошибочных блоков"),
            QStringLiteral("Опыт остановится, как только число блоков с ошибками (обнаруженными и "
                           "пропущенными) достигнет этого значения. Поле доступно в режиме остановки по "
                           "ошибочным блокам."),
            QStringLiteral("целое число от 1 до 1 000 000 000."),
            QStringLiteral("1000 — относительная погрешность Pобн около 3 %."),
            QStringLiteral("чем больше порог, тем точнее оценки вероятностей ошибок.")};
    default:
        return {QStringLiteral("Seed"),
            QStringLiteral("Начальное значение генератора случайных чисел. Одинаковый seed при тех же "
                           "параметрах повторяет опыт бит в бит."),
            QStringLiteral("целое неотрицательное число; пусто или 0 — выбрать случайно."),
            QStringLiteral("42 — чтобы получить тот же результат, что в отчёте."),
            QStringLiteral("на сами вероятности не влияет; использованный seed выводится после опыта.")};
    }
}

QString paragraph(const QString& label, const QString& text)
{
    return QStringLiteral("<p style='margin:4px 0'><b>%1</b> %2</p>").arg(label, text);
}

}

ParameterHelp parameterHelp(HelpTopic topic)
{
    switch (topic) {
    case HelpTopic::Code:
    case HelpTopic::CodeLength:
    case HelpTopic::Generator:
        return codingHelp(topic);
    case HelpTopic::ChannelModel:
        return {QStringLiteral("Модель канала"),
            QStringLiteral("Как канал искажает биты. ДСК — ошибки независимы и равновероятны. Канал Гилберта "
                           "переходит между хорошим и плохим состоянием, поэтому ошибки идут пачками."),
            QStringLiteral("«ДСК (независимые ошибки)» или «Гилберта (пачки ошибок)»."),
            QStringLiteral("канал Гилберта с параметрами по умолчанию: средняя p ≈ 0,005, пачки по 10 бит."),
            QStringLiteral("при той же средней p пачки ошибок искажают меньше блоков, но чаще обманывают "
                           "короткие коды.")};
    case HelpTopic::BitErrorProbability:
        return bscHelp();
    case HelpTopic::GoodToBad:
    case HelpTopic::BadToGood:
    case HelpTopic::ErrorInBad:
    case HelpTopic::ErrorInGood:
        return gilbertHelp(topic);
    default:
        return controlHelp(topic);
    }
}

QString parameterHelpHtml(HelpTopic topic)
{
    const ParameterHelp help = parameterHelp(topic);
    QString html = QStringLiteral("<p style='margin:0 0 4px 0'>%1</p>").arg(help.meaning);
    html += paragraph(QStringLiteral("Допустимо:"), help.allowed);
    html += paragraph(QStringLiteral("Пример:"), help.example);
    if (!help.effect.isEmpty())
        html += paragraph(QStringLiteral("Влияние:"), help.effect);
    return html;
}

QString parameterHelpToolTip(HelpTopic topic)
{
    return QStringLiteral("<div style='max-width:360px'><b>%1</b>%2</div>")
        .arg(parameterHelp(topic).title, parameterHelpHtml(topic));
}

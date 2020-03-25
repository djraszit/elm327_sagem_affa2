# elm327_sagem_affa2

Poprawiona wersja sagem_affa2 z obsługą ELM327. Program dalej będzie rozwijany jak tylko będzie czas.

Po kilku próbach doszedłem do wniosku że komunikacja musi trwać cały czas,
dlatego też wyświetlacz musi być tak jakby odpytywany co kilka milisekund w celu odczytu joysticka.
Mój projekt nie zapewnia odczytu joysticka, aczkolwiek jak ktoś ma joystick to rozgryzie odczyt.
Na chwilę obecną program wyświetla przy pomocy ELM327 obroty silnika, prędkość pojazdu, temperaturę czynnika chłodzącego,
napięcie zasilania na OBD2.
Wyświetla też za pomącą DS18B20 temperaturę wewnątrz pojazdu i na zewnątrz.
Przy pomocy przycisku zmienia się to co chce się odczytywać.

Film na YT
https://www.youtube.com/watch?v=m1o6TgjkzK8

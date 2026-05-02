## MSG_T_READ:
```
                         0                   1                   2                   3
                         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | src_id|res|T|r|       R       |        G      |       B       |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        |                         TIMESTAMP1                            |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        |  nhop |node_id|node_id|...                    |    SYS_CODE   |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
### Pola:
1. `src_id` - id węzła nadającego pomiar (Sx), węzły Sx mają identyfikatory 1xxx
2. `res` - zarezerwowane do przyszłego użytku, aktualnie muszą być ustawione na 0 (jedna z metod weryfikacji, że wiadomości należą do nas)
2. `T` - typ wiadomości (`MSG_T_READ`), odczyt pomiarowy
3. `r` - typ routingu:
- `0` = rozsiewczy, czyli węzeł Wx sprawdza, czy jego `node_id` nie znajduje się na liście, jeśli tak, to ignoruje wiadomość (już ją przesłał), jeśli nie to dodaje swoje node_id do listy, aktualizuje wiadomość i przesyła dalej
- `1` = kierowany, jeżeli węzeł jest na liście i jego `node_id` jest wskazywane przez `nhop` to przesyła wiadomość dalej (zmniejszając o 1 `nhop`), jeśli nie, to ignoruje wiadomość
4. `R`, `G` i `B` - pomiary wartości R, G i B
5. `TIMESTAMP1` - znacznik czasowy uzyskany za pomocą `millis()` (tuż przed wysłaniem), pełni rolę identyfikatora wiadomości
6. `nhop` - ilość przeskoków, które wykonała wiadomość (zastanawiam się, czy dać możliwość dynamicznej zmiany maksymalnej wartości tego parametru, chyba nie, po prostu compile time)
7. `node_id` - lista 4 bitowych identyfikatorów węzłów przekazujących wiadomości w sieci Wx
8. `SYS_CODE` - numer systemu (jedna z metod sprawdzających, czy wiadomość pochodzi z naszego systemu)

### Uwagi:
- Domyślnie używany typ transmisji to tryb rozsiewczy, tryb kierowany używany jest do określania czasu przesłania wiadomości przez sieć (i tak będzie to wartość średnia, bo losowe opóżnienia + ewentualne oczekiwanie na wolną przestrzeń)

## MSG_T_MEAS_RTT:
```
                         0                   1                   2                   3
                         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        | src_id|res|T|r|       R       |        G      |       B       |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        |                         TIMESTAMP1                            |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        |                         TIMESTAMP2                            |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
                        |  nhop |node_id|node_id|...                    |    SYS_CODE   |
                        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
```
### Pola:
- Wszystkie pola jak wyżej przekopiowane przez W0 z tą różnicą, że jest zrobione miejsce na `TIMESTAMP2`
- Przy przekazywaniu przez Wx zmniejszany jest o 1 `nhop`
- Pole `r` jest ustawione na 1 (routing kierowany)
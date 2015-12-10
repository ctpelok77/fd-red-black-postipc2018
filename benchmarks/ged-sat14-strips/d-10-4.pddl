;; original sequence 1: (1 2 3 4 5 6 7 8 -36 -35 -34 -33 -32 -31 -30 -29 -28 -27 -26 -25 -24 -23 -22 -21 -20 -19 -18 -15 -14 -13 -12 -11 -10 -9 40 56 57 58 59 60 37 38 39 -44 -43 -42 -41 45 46 47 48 49 50 51 52 53 16 17 54 55 61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76 -96 -95 -94 -93 -92 -91 -90 -89 -88 -87 -86 -85 -84 -83 -82 -81 -80 -79 -78 -77 -105 -104 -103 -102 -101 -100 -99 -98 -97)
;; original sequence 2: (1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 -76 -75 -74 -73 -72 -71 -70 -69 -68 -67 -66 -65 -64 -63 -62 -61 -60 -59 -58 -57 -56 -39 -38 -37 49 50 51 52 53 40 -35 -34 -33 -32 -31 -30 -29 -28 -27 -26 -44 -43 -42 -41 45 46 47 48 -36 -25 -24 -23 -22 -21 -20 -19 -18 -17 -16 -90 -89 -88 -87 -86 -85 -84 77 78 79 80 81 82 83 91 92 93 94 95 96 -55 -54 -105 -104 -103 -102 -101 -100 -99 -98 -97)
;; simplified sequence 1: (111 -36 107 110 114 40 117 118 109 116 120 119 106 115 112 113 108)
;; simplified sequence 2: (111 -114 -106 -117 -118 116 40 107 109 -36 110 -120 112 -113 -115 -119 108)
;; common subsequences: (((61 62 63 64 65 66 67 68 69 70 71 72 73 74 75 76) . 106) ((-35 -34 -33 -32 -31 -30 -29 -28 -27 -26) . 107) ((-105 -104 -103 -102 -101 -100 -99 -98 -97) . 108) ((-44 -43 -42 -41 45 46 47 48) . 109) ((-25 -24 -23 -22 -21 -20 -19 -18) . 110) ((1 2 3 4 5 6 7 8) . 111) ((-90 -89 -88 -87 -86 -85 -84) . 112) ((-83 -82 -81 -80 -79 -78 -77) . 113) ((-15 -14 -13 -12 -11 -10 -9) . 114) ((-96 -95 -94 -93 -92 -91) . 115) ((49 50 51 52 53) . 116) ((56 57 58 59 60) . 117) ((37 38 39) . 118) ((54 55) . 119) ((16 17) . 120))
;; #safe insertions/deletions: 0
;; sequence 1 (names): ((NORMAL SUB6) (INVERTED G36) (NORMAL SUB2) (NORMAL SUB5) (NORMAL SUB9) (NORMAL G40) (NORMAL SUB12) (NORMAL SUB13) (NORMAL SUB4) (NORMAL SUB11) (NORMAL SUB15) (NORMAL SUB14) (NORMAL SUB1) (NORMAL SUB10) (NORMAL SUB7) (NORMAL SUB8) (NORMAL SUB3))
;; sequence 2 (names): ((NORMAL SUB6) (INVERTED SUB9) (INVERTED SUB1) (INVERTED SUB12) (INVERTED SUB13) (NORMAL SUB11) (NORMAL G40) (NORMAL SUB2) (NORMAL SUB4) (INVERTED G36) (NORMAL SUB5) (INVERTED SUB15) (NORMAL SUB7) (INVERTED SUB8) (INVERTED SUB10) (INVERTED SUB14) (NORMAL SUB3))

(DEFINE (PROBLEM CODONOPSIS-TO-SYMPHYANDRA)
        (:DOMAIN GENOME-EDIT-DISTANCE)
        (:OBJECTS SUB15 SUB14 SUB13 SUB12 SUB11 SUB10 SUB9 SUB8 SUB7
            SUB6 SUB5 SUB4 SUB3 SUB2 SUB1 G40 G36)
        (:INIT (NORMAL SUB6) (INVERTED G36) (NORMAL SUB2) (NORMAL SUB5)
               (NORMAL SUB9) (NORMAL G40) (NORMAL SUB12) (NORMAL SUB13)
               (NORMAL SUB4) (NORMAL SUB11) (NORMAL SUB15)
               (NORMAL SUB14) (NORMAL SUB1) (NORMAL SUB10)
               (NORMAL SUB7) (NORMAL SUB8) (NORMAL SUB3) (PRESENT SUB6)
               (PRESENT G36) (PRESENT SUB2) (PRESENT SUB5)
               (PRESENT SUB9) (PRESENT G40) (PRESENT SUB12)
               (PRESENT SUB13) (PRESENT SUB4) (PRESENT SUB11)
               (PRESENT SUB15) (PRESENT SUB14) (PRESENT SUB1)
               (PRESENT SUB10) (PRESENT SUB7) (PRESENT SUB8)
               (PRESENT SUB3) (CW SUB3 SUB6) (CW SUB8 SUB3)
               (CW SUB7 SUB8) (CW SUB10 SUB7) (CW SUB1 SUB10)
               (CW SUB14 SUB1) (CW SUB15 SUB14) (CW SUB11 SUB15)
               (CW SUB4 SUB11) (CW SUB13 SUB4) (CW SUB12 SUB13)
               (CW G40 SUB12) (CW SUB9 G40) (CW SUB5 SUB9)
               (CW SUB2 SUB5) (CW G36 SUB2) (CW SUB6 G36) (IDLE)
               (= (TOTAL-COST) 0))
        (:GOAL (AND (NORMAL SUB6) (INVERTED SUB9) (INVERTED SUB1)
                    (INVERTED SUB12) (INVERTED SUB13) (NORMAL SUB11)
                    (NORMAL G40) (NORMAL SUB2) (NORMAL SUB4)
                    (INVERTED G36) (NORMAL SUB5) (INVERTED SUB15)
                    (NORMAL SUB7) (INVERTED SUB8) (INVERTED SUB10)
                    (INVERTED SUB14) (NORMAL SUB3) (CW SUB3 SUB6)
                    (CW SUB14 SUB3) (CW SUB10 SUB14) (CW SUB8 SUB10)
                    (CW SUB7 SUB8) (CW SUB15 SUB7) (CW SUB5 SUB15)
                    (CW G36 SUB5) (CW SUB4 G36) (CW SUB2 SUB4)
                    (CW G40 SUB2) (CW SUB11 G40) (CW SUB13 SUB11)
                    (CW SUB12 SUB13) (CW SUB1 SUB12) (CW SUB9 SUB1)
                    (CW SUB6 SUB9)))
        (:METRIC MINIMIZE (TOTAL-COST)))
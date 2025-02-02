package zapoctovy_test;

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

public class main {

    static char[] prunikSlov(char[] prvniSlovo, char[] druheSlovo) {
        int prvniDelka = prvniSlovo.length;
        int druhaDelka = druheSlovo.length;
        int maximalniDelka;
        if (prvniDelka > druhaDelka) {
            maximalniDelka = druhaDelka;
        } else {
            maximalniDelka = prvniDelka;
        }
        char[] spolecneTokeny = new char[maximalniDelka];
        int indexSpolecnychZnaku = 0;
        char znakPrvnihoPole;
        for (int i = 0; i < prvniSlovo.length; i++) {
            znakPrvnihoPole = prvniSlovo[i];
            if (obsahujeZnak(znakPrvnihoPole, spolecneTokeny) == false && obsahujeZnak(znakPrvnihoPole, druheSlovo) == true) {

                spolecneTokeny[indexSpolecnychZnaku] = znakMalymiPismeny(znakPrvnihoPole);
                indexSpolecnychZnaku++;
            }
        }
        return spolecneTokeny;
    } // metoda, ktera by mela najit prunikslov, nevim, proc, ale je nejaka chyba v ukladani znaku do pole

    static boolean obsahujeZnak(char znak, char[] testovaneSlovo) {
        for (int i = 0; i < testovaneSlovo.length; i++) {
            if (rovnostZnaku(znak, testovaneSlovo[i])) {
                return true;
            }
        }
        return false;
    // metoda obsahujeZnak zjisti, jestli dane pole obsahuje zadany znak
    }

    static void vypisPole(char[] pole) {
        for (int i = 0; i < pole.length; i++) {
            if (!rovnostZnaku(' ', pole[i])) {
                System.out.println(pole[i]);
            }
        }
    }         // metoda pro vypisn pole

    static boolean rovnostZnaku(char znak1, char znak2) {

        if (((int) znakMalymiPismeny(znak1) == (int) znakMalymiPismeny(znak2))) {
            return true;
        }
        return false;
    }

    static char znakMalymiPismeny(char znak) {
        if ((int) znak <= (int) 'Z' && (int) znak >= (int) ('A')) {
            return (char) ((int) (znak) - (int) ('A') + (int) ('a'));
        }
        return znak;
    }

    /**
     * @param args the command line arguments
     */
    public static void main(String[] args) {
        char[] betka = new char[4];
        betka[1] = 'e';
        betka[2] = 'e';
        betka[3] = 'e';
        betka[0] = 'I';
        char[] beta = new char[4];
        beta[1] = 'i';
        beta[0] = 'E';
        beta[2] = 'i';
        beta[3] = 'E';

        vypisPole(prunikSlov(betka, beta));
        System.out.println((obsahujeZnak('e', betka)));

    }
}
package alg12;

import java.util.*;

public class Zavorky {
  public static void main(String[] args) {
     Scanner sc = new Scanner(System.in);
     ZasobnikZnaku zasobnik = new ZasobnikZnaku();
//    ZasobnikZnaku2 zasobnik = new ZasobnikZnaku2();
  //  ZasobnikZnaku3 zasobnik = new ZasobnikZnaku3();
    System.out.println("zadejte ��dek se z�vorkami");
    String str = sc.nextLine();
    int i;
    for (i=0; i<str.length(); i++) {
      char znak = str.charAt(i);
      System.out.print(znak);
      if (jeOteviraci(znak))
        zasobnik.vloz(znak);
      else if (jeZaviraci(znak)) {
        if (zasobnik.jePrazdny())
          chyba("k t�to z�vorce chyb� otev�rac�");
        char ocekavany = zaviraciK(zasobnik.odeber());
        if (znak!=ocekavany)
          chyba("o�ek�v� se "+ocekavany);
      }
    }
    if (!zasobnik.jePrazdny()) {
      String chybi = "";
      do {
        chybi = chybi + zasobnik.odeber();
      } while (!zasobnik.jePrazdny());
      chyba("chyb� zav�rac� z�vorky k "+chybi);
    }
    System.out.println();
  }

  static boolean jeOteviraci(char z) {
    return z=='(' || z=='[' || z=='{';
  }

  static boolean jeZaviraci(char z) {
    return z==')' || z==']' || z=='}';
  }

  static char zaviraciK(char z) {
    if (z=='(') return ')';
    if (z=='[') return ']';
    if (z=='{') return '}';
    return z;
  }

  static void chyba(String str) {
    System.out.println(" chyba: "+str);
    System.exit(0);
  }
}


package alg5;
import java.util.*;

public class Nim {
  static int pocet;     // aktu�ln� po�et z�palek
  static boolean stroj; // =true znamen�, �e bere po��ta�

  public static void main(String[] args) {
    zadaniPoctu();
    stroj = false;  // zacina hrac
    do {
      if (stroj) bereStroj(); else bereHrac();
      stroj = !stroj;
    } while (pocet>0);
    if (stroj)
      System.out.println("vyhr�l jsem");
    else
      System.out.println("vyhr�l jste, gratuluji");
  }

  static void zadaniPoctu() {
       Scanner sc = new Scanner(System.in);
      do {
      System.out.println("zadejte po�et z�palek (od 15 do 35)");
      pocet = sc.nextInt();
    } while (pocet<15 || pocet>30);
  }

  static void bereHrac() {
    Scanner sc = new Scanner(System.in);
      int x;
    boolean chyba;
    do {
      chyba = false;
      System.out.println("po�et z�palek "+pocet);
      System.out.println("kolik odeberete");
      x = sc.nextInt();
      if (x<1) {
        System.out.println("prilis malo");
        chyba = true;
      }
      else
      if (x>3 || x>pocet) {
        System.out.println("prilis mnoho");
        chyba = true;
      }
    } while (chyba);
    pocet -= x;
  }

  static void bereStroj() {
    System.out.println("po�et z�palek "+pocet);
    int x = (pocet-1) % 4;
    if (x==0) x = 1;
    System.out.println("odeb�r�m "+x);
    pocet -= x;
  }

}
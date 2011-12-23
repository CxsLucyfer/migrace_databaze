package alg13;

import java.util.*;


public class Hra {
  public static void main(String[] args) {

   Uzel koren = inicializaceStromu();
    for (;;) {
      System.out.println("Mysl�te si n�jak� zv��e?");
      if (!odpovedAno()) break;
      Uzel aktualni = koren;
      do {
        System.out.println(aktualni.text);
        if (odpovedAno()) aktualni = aktualni.ano;
        else aktualni = aktualni.ne;
      } while (!aktualni.jeList());
      System.out.println("Je to "+aktualni.text+"?");
      if (odpovedAno()) System.out.println("Uh�dl jsem");
      else {
        System.out.println("Neuh�dl jsem. Pros�m o dopln�n� znalost�");
        doplnPodstrom(aktualni);
      }
      System.out.println("D�kuji. Chcete pokra�ovat?");
      if (!odpovedAno()) break;
    }
  }

  static boolean odpovedAno() {
       Scanner sc = new Scanner(System.in);
       String s = sc.nextLine();
    if (s.length()>0 && (s.charAt(0)=='a' || s.charAt(0)=='A'))
      return true;
    else
      return false;
  }

  static Uzel inicializaceStromu() {
    return new Uzel("l�t�?",
                    new Uzel("pt�k", null, null),
                    new Uzel("ryba", null, null));
  }

  static void doplnPodstrom(Uzel p) {
       Scanner sc = new Scanner(System.in);
       String noveZvire, novaOtazka;
    Uzel novyAno, novyNe;
    System.out.println("Jak� zvi�e jste myslel?");
    noveZvire = sc.nextLine();
    System.out.println("Napi�te ot�zku vystihuj�c� rozd�l mezi "+noveZvire+" a "+p.text);
    novaOtazka = sc.nextLine();
    System.out.println("Pro zv��e, kter� jste myslel, je odpove� ano �i ne");
    if (odpovedAno()) {
      novyAno = new Uzel(noveZvire); novyNe = new Uzel(p.text);
    } else {
      novyAno = new Uzel(p.text); novyNe = new Uzel(noveZvire);
    }
    p.text = novaOtazka;
    p.ano = novyAno;
    p.ne = novyNe;
  }
}

class Uzel {
  String text;
  Uzel ano, ne;

  public Uzel(String t) {
    text = t; ano = null; ne = null;
  }

  public Uzel(String t, Uzel a, Uzel n) {
    text = t; ano = a; ne = n;
  }

  public boolean jeList() {
    return ano==null && ne==null;
  }
}
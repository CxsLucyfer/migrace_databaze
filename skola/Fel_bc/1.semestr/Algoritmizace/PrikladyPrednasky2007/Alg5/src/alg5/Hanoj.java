package alg5;
import java.util.*;

public class Hanoj {
  public static void main(String[] args) {
      Scanner sc = new Scanner(System.in);
      System.out.println("zadejte v��ku v�e");
    int pocetDisku = sc.nextInt();
    prenesVez(pocetDisku, 1, 2, 3);
  }

  static void prenesVez(int vyska, int odkud, int kam, int pomoci) {
    if (vyska>0) {
      prenesVez(vyska-1, odkud, pomoci, kam);
      System.out.println("p�enes disk z "+odkud+" na "+kam);
      prenesVez(vyska-1, pomoci, kam, odkud);
    }
  }

}
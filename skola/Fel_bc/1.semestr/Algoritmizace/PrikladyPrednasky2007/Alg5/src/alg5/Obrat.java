package alg5;
import java.util.*;

public class Obrat {
  public static void main(String[] args) {
    System.out.println("zadejte posloupnost cel�ch ��sel zakon�enou nulou");
    obrat();
  }

  static void obrat() {
     Scanner sc = new Scanner(System.in);
     int x = sc.nextInt();
    if (x!=0) obrat();
   System.out.print(x + " ");
  }

}
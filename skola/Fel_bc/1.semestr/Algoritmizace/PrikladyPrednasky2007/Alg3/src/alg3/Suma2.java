package alg3;
import java.util.*;

public class Suma2 {
  public static void main(String[] args) {
      Scanner sc = new Scanner(System.in);
      int dalsi, suma, i, n;
   System.out.println("zadejte po�et ��sel");
    n = sc.nextInt();
    System.out.println("zadejte " + n +" ��sel");
    suma = 0;
    for (i=1; i<=n; i++) {
      dalsi = sc.nextInt();
      suma = suma+dalsi;
    }
    System.out.println("sou�et je " + suma);
  }
}
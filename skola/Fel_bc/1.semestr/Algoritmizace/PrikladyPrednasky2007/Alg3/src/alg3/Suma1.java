package alg3;
import java.util.*;

public class Suma1 {
  public static void main(String[] args) {
     Scanner sc = new Scanner(System.in);
     int dalsi, suma, i;
    System.out.println("zadejte 5 ��sel");
    suma = 0;
    for (i=1; i<=5; i++) {
      dalsi = sc.nextInt();
      suma = suma+dalsi;
    }
    System.out.println("sou�et je " + suma);
  }
}
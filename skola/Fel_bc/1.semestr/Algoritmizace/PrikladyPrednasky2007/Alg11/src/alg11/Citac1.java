package alg11;

 import java.util.*;

public class Citac1 {
  final static int pocHodn = 0;
  static int hodn, volba;
  public static void main(String[] args) {
   Scanner sc = new Scanner(System.in);
   hodn = pocHodn;
    do {
      System.out.println("Hodnota = "+hodn);
      System.out.println("0) Konec\n1) Zv�t�it\n2) Zmen�it\n3) Nastavit");
      System.out.print("Va�e volba: ");
      volba = sc.nextInt();
      switch (volba) {
        case 0: break;
        case 1: hodn++; break;
        case 2: hodn--; break;
        case 3: hodn = pocHodn; break;
        default: System.out.println("Nedovolen� volba");
      }
    } while (volba>0);
    System.out.println("Konec");
  }
}

public class Main {
	public static void main(String [] args){
		Edge []edges = {
				new Edge(0, 0, 1, 10),
				new Edge(0, 2, 3, 10),
				new Edge(0, 3, 4, 10),
				new Edge(0, 4, 5, 10),
				new Edge(0, 5, 6, 10),
				new Edge(0, 6, 7, 10),
				new Edge(0, 7, 8, 10),
				new Edge(0, 8, 9, 10),
				new Edge(0, 9, 10, 10),
				new Edge(0, 1, 2, 10)
				
		};
		
		BinomialHeap heap = new BinomialHeap(edges[9]);
		for(int i = 8 ; i >= 0 ; i--){
			heap.insert(edges[i]);
		}
		
		
		
		for(int i = 0 ; i < 10; i++){
			System.out.println(heap.accessMin());
			heap.deleteMin();
		}
		System.out.println("ende");
	}
}

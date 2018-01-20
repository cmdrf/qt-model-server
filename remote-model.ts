import { Observable } from 'rxjs/Observable';
import { of } from 'rxjs/observable/of';

export class RemoteModel {
  private items: any = {};
  private socket: WebSocket;
  
  constructor(private url: string) { }

  getItems(): Observable<any[]> {
    this.socket = new WebSocket(this.url);
    let observable = Observable.create(
      observer => {
          this.socket.onmessage = ((msg) => {
            var obj = JSON.parse(msg.data);

            for(var id in obj.items)
              obj.items[id].id = id;

            if(obj.operation == "data") {
              this.items = obj.items
            }
            else if(obj.operation == "inserted") {
              for(var id in obj.items) {
                var item = obj.items[id];
                this.items[id] = item;
              }
            }
            else if(obj.operation == "removed") {
              // TODO
            }
            else if(obj.operation == "dataChanged") {
              for(var id in obj.items) {
                var item = obj.items[id];
                if(this.items.hasOwnProperty(id))
                  this.items[id] = item;
              }
            }
            observer.next(Object.keys(this.items).map(key => this.items[key])); // TODO: Sort         
          });
          this.socket.onerror = observer.error.bind(observer);
          this.socket.onclose = observer.complete.bind(observer);
          return this.socket.close.bind(this.socket);
      }
    );
    return observable;
  }

  editItem(item: any) {
    let id: string = String(item.id);
    let items = {};
    items[id] = item;
    let msg = {
      operation: "changeData",
      items: items
    };
    this.socket.send(JSON.stringify(msg));
  }
}

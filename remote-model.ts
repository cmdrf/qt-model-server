import { BehaviorSubject } from 'rxjs';

export class RemoteModel {
  private items: any;
  private keyItem: string = "id";
  private socket: WebSocket;
  private subject: BehaviorSubject<any[]> = new BehaviorSubject([]);
  
  constructor(private url: string) {
    this.socket = new WebSocket(this.url);
    this.socket.onmessage = ((msg) => {
      var obj = JSON.parse(msg.data);

      if(obj.operation == "data") {
        this.items = obj.items;
      }
      if(obj.operation == "rowData") {
        this.items = obj.items;
        this.keyItem = obj.key;
      }
      else if(obj.operation == "inserted") {
        for(var id in obj.items) {
          var item = obj.items[id];
          this.items[id] = item;
        }
      }
      else if(obj.operation == "rowsInserted") {
        this.items.splice(obj.start, 0, ...obj.items);
      }
      else if(obj.operation == "removed") {
        for(var id in obj.items) {
          delete this.items[id];
        }
      }
      else if(obj.operation == "rowsRemoved") {
        this.items.splice(obj.start, obj.end - obj.start + 1);
      }
      else if(obj.operation == "dataChanged") {
        for(var id in obj.items) {
          var item = obj.items[id];
          if(this.items.hasOwnProperty(id))
            this.items[id] = item;
        }
      }
      else if(obj.operation == "rowDataChanged") {
        this.items.splice(obj.start, obj.end - obj.start + 1, ...obj.items);

      }
      if(Array.isArray(this.items))
        this.subject.next(this.items);
      else
        this.subject.next(Object.keys(this.items).map(key => this.items[key]));       
    });
    this.socket.onerror = this.subject.error.bind(this.subject);
    this.socket.onclose = this.subject.complete.bind(this.subject);
  }

  getItems(): BehaviorSubject<any[]> {
    return this.subject;
  }

  editItem(item: any) {
    let id: string = String(item[this.keyItem]);
    let items = {};
    items[id] = item;
    let msg = {
      operation: "changeData",
      items: items
    };
    this.socket.send(JSON.stringify(msg));
  }

  removeItem(id: string) {
    let msg = {
      operation: "remove",
      items: [id]
    };
    this.socket.send(JSON.stringify(msg));
  }

  insertItem(item: any) {
    let items;
    if(item.hasOwnProperty(this.keyItem)) {
      items = {};
      items[String(item[this.keyItem])] = item;
    }
    else {
      items = [item];
    }
    let msg = {
      operation: "insert",
      items: items
    };
    this.socket.send(JSON.stringify(msg));
  }
}

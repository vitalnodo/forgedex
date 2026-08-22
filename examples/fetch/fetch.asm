; ══════════════════════════════════════════════════════════
;  Fetch — a page off the network, shown on a second screen
;
;  class Main extends Activity implements View.OnClickListener {
;      EditText field;                       // holds the address
;      public void onClick(View v) {
;          startActivity(new Intent(this, Page.class)
;                            .putExtra("url", field.getText().toString()));
;      }
;  }
;
;  class Page extends Activity {
;      public void onCreate(Bundle b) {
;          ...a TextView inside a ScrollView...
;          Worker w = new Worker();
;          w.tv = theTextView;
;          w.url = getIntent().getStringExtra("url");
;          new Thread(w).start();
;      }
;  }
;
;  class Worker implements Runnable {
;      TextView tv; String url, result; int stage;
;      public void run() {
;          if (stage == 0) {                 // on the worker thread
;              stage = 1;
;              result = ...read the page...;
;              tv.post(this);                // hand back to the UI thread
;          } else {                          // on the UI thread
;              tv.setText(result);
;          }
;      }
;  }
;
;  One Runnable does both halves by looking at stage and reposting
;  itself, the same trick the game loop uses. A request cannot run on
;  the UI thread — that throws NetworkOnMainThreadException — so the
;  first half runs on a Thread and the second is posted back.
;
;  The address is https, deliberately: cleartext http is refused by
;  default from API 28 onwards, and turning it back on would mean
;  another attribute marco does not know.
;
;  There is no try/catch in the DSL yet, so a network error has
;  nowhere to go and takes the process with it. This is the happy
;  path only.
; ══════════════════════════════════════════════════════════

format binary as "dex"
org $00

include '../../forgedex.inc'

macro _types
    deftype "Ljava/lang/Object;"
end macro

macro _main
    defclass "Lapp/hello/fetch/Main;", ACC_PUBLIC, \
        "Landroid/app/Activity;", _main_ifaces

    deffield "->field:Landroid/widget/EditText;", ACC_PUBLIC

    method "-><init>()V", ACC_PUBLIC, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    ; v0=layout  v1=view  v2=scratch
    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 3
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1

        new_instance   v0, "Landroid/widget/LinearLayout;"
        invoke_direct  "Landroid/widget/LinearLayout;-><init>(Landroid/content/Context;)V", v0, p0
        const_4        v2, 1
        invoke_virtual "Landroid/widget/LinearLayout;->setOrientation(I)V", v0, v2

        new_instance   v1, "Landroid/widget/TextView;"
        invoke_direct  "Landroid/widget/TextView;-><init>(Landroid/content/Context;)V", v1, p0
        const_string   v2, "address:"
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v1, v2
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v0, v1

        new_instance   v1, "Landroid/widget/EditText;"
        invoke_direct  "Landroid/widget/EditText;-><init>(Landroid/content/Context;)V", v1, p0
        const_string   v2, "https://example.com"
        invoke_virtual "Landroid/widget/EditText;->setText(Ljava/lang/CharSequence;)V", v1, v2
        iput_object    v1, p0, "->field:Landroid/widget/EditText;"
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v0, v1

        new_instance   v1, "Landroid/widget/Button;"
        invoke_direct  "Landroid/widget/Button;-><init>(Landroid/content/Context;)V", v1, p0
        const_string   v2, "мені пощастить"
        invoke_virtual "Landroid/widget/Button;->setText(Ljava/lang/CharSequence;)V", v1, v2
        invoke_virtual "Landroid/widget/Button;->setOnClickListener(Landroid/view/View$OnClickListener;)V", v1, p0
        invoke_virtual "Landroid/widget/LinearLayout;->addView(Landroid/view/View;)V", v0, v1

        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v0
        return_void
    end method

    ; v0=Intent  v1=scratch  v2=address
    method "->onClick(Landroid/view/View;)V", ACC_PUBLIC, locals 3
        new_instance   v0, "Landroid/content/Intent;"
        const_class    v1, "Lapp/hello/fetch/Page;"
        invoke_direct  "Landroid/content/Intent;-><init>(Landroid/content/Context;Ljava/lang/Class;)V", v0, p0, v1

        iget_object    v1, p0, "->field:Landroid/widget/EditText;"
        invoke_virtual "Landroid/widget/EditText;->getText()Landroid/text/Editable;", v1
        move_result_object v1
        invoke_virtual "Ljava/lang/Object;->toString()Ljava/lang/String;", v1
        move_result_object v2
        const_string   v1, "url"
        invoke_virtual "Landroid/content/Intent;->putExtra(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;", v0, v1, v2
        move_result_object v0

        invoke_virtual "Landroid/app/Activity;->startActivity(Landroid/content/Intent;)V", p0, v0
        return_void
    end method
end macro

macro _page
    defclass "Lapp/hello/fetch/Page;", ACC_PUBLIC, "Landroid/app/Activity;"

    method "-><init>()V", ACC_PUBLIC, locals 0
        invoke_direct  "Landroid/app/Activity;-><init>()V", p0
        return_void
    end method

    ; v0=scroller  v1=text view  v2=worker  v3,v4=scratch
    method "->onCreate(Landroid/os/Bundle;)V", ACC_PUBLIC, locals 5
        invoke_super   "Landroid/app/Activity;->onCreate(Landroid/os/Bundle;)V", p0, p1

        new_instance   v0, "Landroid/widget/ScrollView;"
        invoke_direct  "Landroid/widget/ScrollView;-><init>(Landroid/content/Context;)V", v0, p0
        new_instance   v1, "Landroid/widget/TextView;"
        invoke_direct  "Landroid/widget/TextView;-><init>(Landroid/content/Context;)V", v1, p0
        const_string   v3, "fetching..."
        invoke_virtual "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v1, v3
        invoke_virtual "Landroid/widget/ScrollView;->addView(Landroid/view/View;)V", v0, v1
        invoke_virtual "Landroid/app/Activity;->setContentView(Landroid/view/View;)V", p0, v0

        new_instance   v2, "Lapp/hello/fetch/Worker;"
        invoke_direct  "Lapp/hello/fetch/Worker;-><init>()V", v2
        iput_object    v1, v2, "Lapp/hello/fetch/Worker;->tv:Landroid/widget/TextView;"

        invoke_virtual "Landroid/app/Activity;->getIntent()Landroid/content/Intent;", p0
        move_result_object v3
        const_string   v4, "url"
        invoke_virtual "Landroid/content/Intent;->getStringExtra(Ljava/lang/String;)Ljava/lang/String;", v3, v4
        move_result_object v3
        iput_object    v3, v2, "Lapp/hello/fetch/Worker;->url:Ljava/lang/String;"

        new_instance   v3, "Ljava/lang/Thread;"
        invoke_direct  "Ljava/lang/Thread;-><init>(Ljava/lang/Runnable;)V", v3, v2
        invoke_virtual "Ljava/lang/Thread;->start()V", v3
        return_void
    end method
end macro

macro _worker
    defclass "Lapp/hello/fetch/Worker;", ACC_PUBLIC, \
        "Ljava/lang/Object;", _worker_ifaces

    deffield "->tv:Landroid/widget/TextView;", ACC_PUBLIC
    deffield "->url:Ljava/lang/String;",       ACC_PUBLIC
    deffield "->result:Ljava/lang/String;",    ACC_PUBLIC
    deffield "->stage:I",                      ACC_PUBLIC

    method "-><init>()V", ACC_PUBLIC, locals 0
        invoke_direct  "Ljava/lang/Object;-><init>()V", p0
        return_void
    end method

    ; v0=line/scratch  v2=URL  v3=stream  v4=reader  v5=buffered
    ; v6=builder  v7=line count  v8=scratch
    method "->run()V", ACC_PUBLIC, locals 9
        iget            v0, p0, "->stage:I"
        if_nez          v0, .deliver

        const_4         v0, 1
        iput            v0, p0, "->stage:I"

        iget_object     v1, p0, "->url:Ljava/lang/String;"
        new_instance    v2, "Ljava/net/URL;"
        invoke_direct   "Ljava/net/URL;-><init>(Ljava/lang/String;)V", v2, v1
        invoke_virtual  "Ljava/net/URL;->openStream()Ljava/io/InputStream;", v2
        move_result_object v3
        new_instance    v4, "Ljava/io/InputStreamReader;"
        invoke_direct   "Ljava/io/InputStreamReader;-><init>(Ljava/io/InputStream;)V", v4, v3
        new_instance    v5, "Ljava/io/BufferedReader;"
        invoke_direct   "Ljava/io/BufferedReader;-><init>(Ljava/io/Reader;)V", v5, v4
        new_instance    v6, "Ljava/lang/StringBuilder;"
        invoke_direct   "Ljava/lang/StringBuilder;-><init>()V", v6
        const_4         v7, 0

    .read_line:
        invoke_virtual  "Ljava/io/BufferedReader;->readLine()Ljava/lang/String;", v5
        move_result_object v0
        if_eqz          v0, .page_read
        invoke_virtual  "Ljava/lang/StringBuilder;->append(Ljava/lang/String;)Ljava/lang/StringBuilder;", v6, v0
        const           v8, 10                                 ; a newline
        invoke_virtual  "Ljava/lang/StringBuilder;->append(C)Ljava/lang/StringBuilder;", v6, v8
        add_int_lit8    v7, v7, 1
        const           v8, 200                                ; enough of any page
        if_ge           v7, v8, .page_read
        goto            .read_line

    .page_read:
        invoke_virtual  "Ljava/io/BufferedReader;->close()V", v5
        invoke_virtual  "Ljava/lang/StringBuilder;->toString()Ljava/lang/String;", v6
        move_result_object v0
        iput_object     v0, p0, "->result:Ljava/lang/String;"
        iget_object     v0, p0, "->tv:Landroid/widget/TextView;"
        invoke_virtual  "Landroid/view/View;->post(Ljava/lang/Runnable;)Z", v0, p0
        return_void

    .deliver:
        iget_object     v0, p0, "->tv:Landroid/widget/TextView;"
        iget_object     v1, p0, "->result:Ljava/lang/String;"
        invoke_virtual  "Landroid/widget/TextView;->setText(Ljava/lang/CharSequence;)V", v0, v1
        return_void
    end method
end macro

macro _interfaces
    _main_ifaces   deftypelist "Landroid/view/View$OnClickListener;"
    _worker_ifaces deftypelist "Ljava/lang/Runnable;"
end macro

dex_lists _main, _page, _worker, _types

dex_file _interfaces

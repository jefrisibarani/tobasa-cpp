    <meta charset="utf-8" />
    <meta http-equiv="X-UA-Compatible" content="IE=edge" />
    <meta name="viewport" content="width=device-width, initial-scale=1, shrink-to-fit=no" />
    <meta name="description" content="" />
    <meta name="author" content="" />

    {% if not exists("statusCode") or statusCode != "500" %}
    <meta name="csrf-token" content="{{ default(csrfToken, "") }}" />
    {% endif %}
<template>
  <!-- This example requires Tailwind CSS v2.0+ -->
  <nav class="flex mb-2" aria-label="Breadcrumb">
    <ol role="list" class="flex items-center space-x-3">
      <li>
        <div class="flex items-center">
          <a
            href="#"
            class="
              ml-2
              text-sm
              font-medium
              text-sl-disabled
              hover:text-sl-on_surface_2
            "
            >Rooms</a
          >
        </div>
      </li>

      <li>
        <div class="flex items-center">
          <!-- Heroicon name: solid/chevron-right -->
          <svg
            class="flex-shrink-0 h-5 w-5 text-sl-disabled"
            xmlns="http://www.w3.org/2000/svg"
            viewBox="0 0 20 20"
            fill="currentColor"
            aria-hidden="true"
          >
            <path
              fill-rule="evenodd"
              d="M7.293 14.707a1 1 0 010-1.414L10.586 10 7.293 6.707a1 1 0 011.414-1.414l4 4a1 1 0 010 1.414l-4 4a1 1 0 01-1.414 0z"
              clip-rule="evenodd"
            />
          </svg>
          <span
            class="ml-3 text-sm font-medium text-sl-disabled"
            aria-current="page"
            >Podcast xyz</span
          >
        </div>
      </li>

      <li>
        <div class="flex items-center">
          <a
            title="New room"
            href="#"
            class="
              ml-2
              font-medium
              text-lg text-sl-disabled
              hover:text-sl-on_surface_2
            "
            aria-current="page"
            >+</a
          >
        </div>
      </li>
    </ol>
  </nav>
</template>

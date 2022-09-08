<template>
    <nav class="flex mb-2" aria-label="Breadcrumb">
        <ol role="list" class="flex items-center space-x-3">
            <li>
                <div class="flex items-center">
                    <a href="#" class="
              text-sm
              font-medium
              text-sl-disabled
              hover:text-sl-on_surface_2
            ">Room</a>
                </div>
            </li>

            <li>
                <div class="flex items-center">
                    <!-- Heroicon name: solid/chevron-right -->
                    <svg class="flex-shrink-0 h-5 w-5 text-sl-disabled" xmlns="http://www.w3.org/2000/svg"
                        viewBox="0 0 20 20" fill="currentColor" aria-hidden="true">
                        <path fill-rule="evenodd"
                            d="M7.293 14.707a1 1 0 010-1.414L10.586 10 7.293 6.707a1 1 0 011.414-1.414l4 4a1 1 0 010 1.414l-4 4a1 1 0 01-1.414 0z"
                            clip-rule="evenodd" />
                    </svg>

                    <button type="button"
                        class="ml-3 text-sl-disabled group inline-flex items-center rounded-md text-sm font-medium hover:text-sl-on_surface_1"
                        aria-expanded="false">
                        <span>Default</span>
                        <!--
      Heroicon name: mini/chevron-down

      Item active: "text-gray-600", Item inactive: "text-gray-400"
    -->
                        <svg class="text-sl-disabled ml-2 h-5 w-5 group-hover:text-sl-on_surface_1"
                            xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor"
                            aria-hidden="true">
                            <path fill-rule="evenodd"
                                d="M5.23 7.21a.75.75 0 011.06.02L10 11.168l3.71-3.938a.75.75 0 111.08 1.04l-4.25 4.5a.75.75 0 01-1.08 0l-4.25-4.5a.75.75 0 01.02-1.06z"
                                clip-rule="evenodd" />
                        </svg>
                    </button>
                </div>
            </li>
        </ol>
    </nav>
</template>
